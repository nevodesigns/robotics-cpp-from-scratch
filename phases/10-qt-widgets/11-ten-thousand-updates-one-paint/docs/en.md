# Ten Thousand Updates, One Paint: update() and repaint(), and Drawing Only What Changed

> update() says the picture is stale and paints once, whenever the loop is free. repaint() paints now, every time. Tell a paint how much changed and it can do less.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 10-03, 10-04

## The Problem

Lesson 10-03 separated the rate data arrives at from the rate a screen is drawn at. This is the Qt that does the separating, and the part of it that is easy to get wrong in either direction: a call that paints too often, and a paint that does too much of the chart each time.

## The Concept

### update() and repaint()

A chart that adds 10000 samples and calls `update` for each: **0** paints inside the calls, and **1** after the event loop ran. A chart that calls `repaint()` 100 times: **100** paints, at once. 100 calls of `update()`: **1**. See `E-QT-0057`.

`update()` is a request: the picture is out of date. Qt keeps one such request for a widget, and paints when the loop is free. `repaint()` is an order, and each one is a paint.

### Only the column that changed

`update(rect)` marks a rectangle, and the paint event carries it. A full update of this 300 column chart went over **300** columns. One new sample, with `update(column_rect(...))`, gave **1** paint of a rectangle at x 0, **1** pixel wide and **100** high, and went over **1** column. See `E-QT-0058`.

The paint event only helps if the code uses it: a `paintEvent` that draws all 300 columns every time paints the same pixels over and over, and Qt clips them away after the work is done.

### Two places at once

Updates of columns 10 and 200 together were **1** paint of the rectangle from x = 10 to x = 200, over **191** columns. The dirty region is a set of rectangles, but the event's rectangle is the one that bounds them, so two small changes far apart cost nearly the width between them. See `E-QT-0059`.

## Build It

`column_rect`, `add_sample` and `paintEvent` of `ChartWidget` in `exercise/solution.hpp`.

```
rcpp verify 10-11
```

## Use It

`rc/qt/dirty_paint.hpp` joins its module.

```cpp
rc::qt::ChartWidget chart;
chart.show();
// from a timer, or a queued signal on the widget's own thread:
chart.add_sample(reading);
```

**What this does not do.** It is a strip chart of fixed size, so the wrap at the right edge redraws a column in place, and a chart that scrolls moves every column and needs the whole width (lesson 10-03). The counts of paints are for the offscreen platform in Qt 6.2 to 6.5, and a platform that composites its own windows may repaint more than it is asked. It does not draw from a thread: a widget is only painted by the thread that owns it (lesson 09-13).

## What Breaks First

- **A paint for every sample.** See `E-QT-0057`.
- **A whole chart redrawn for one column.** See `E-QT-0058`.
- **A bounding rectangle that covers the gap.** See `E-QT-0059`.

## Ship It

`rc/qt/dirty_paint.hpp` holds `ChartWidget`.

Lesson 10-03 drew when a timer said so. This is what to call when something has changed, and how to say how much.
