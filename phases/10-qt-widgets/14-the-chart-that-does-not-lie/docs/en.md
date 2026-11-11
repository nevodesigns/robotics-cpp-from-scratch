# The Chart That Does Not Lie: One Model, a Window and a Terminal, and a Steady Signal That Stays Steady

> One model works out the range, and both the window and the terminal draw from it. Then a steady signal is flat in both, and a spike is tall in both.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 10-13, 10-11, 10-08, 10-07, 10-06, 10-05

## The Problem

Phase 10 has built the parts of a chart one at a time: ticks, columns, a range, a map from data to pixels, a layout, a repaint, a picture comparison, a palette. This puts them behind one model, so that the chart in a window and the chart in a terminal are the same chart, and a signal that is steady is shown as steady in both.

## The Concept

### One model

`ChartModel` holds the samples, the window of time and the minimum span. Its `range()` is worked out once, from the samples in the window, by one column that holds the whole window (the lowest and highest of every sample in it), with room around it and never narrower than the minimum span. `text()` is the terminal chart of lesson 10-07 and `view()` is the map of lesson 10-08. `SignalChart` is a widget whose `paintEvent` draws from the same model, and whose `add_sample` asks for a repaint and does not force one (lesson 10-11).

### A steady signal

5 plus or minus 1e-6, a minimum span of 1. The text marked **1** of 13 rows. The window rendered into a picture 200 by 130 had ink on **1** of 130 pixel rows, row **65**, the middle. The model's range was 1 wide and centred on 5. See `E-QT-0063`.

### The same lie, together

With no minimum span the text marked **11** of 13 rows and the window had ink on **119** of 130 pixel rows. A chart that scales to its data shows 2e-6 of noise as a storm, and both views do, because they use one range. The remedy is in the model, once, and not in each view.

### A spike

10000 quiet samples with a spike of 5 at sample 7000. The text's highest mark was in column **35** of 50 and the window's top ink was in column **140** of 200: 0.71 and 0.70 of the way across, within one text column of each other. The spike was **119** pixels tall. See `E-QT-0064`.

### Ticks

For a sine from -0.99919 to 20.9999 the model had **5** ticks, and all **5** were marked at the left of the window. The text had **3** labels, and all **3** were ticks of the same range. See `E-QT-0065`.

### The edges

An empty chart has no ink. A window with no width in time gives a finite range and still draws. Samples added through the widget are in the model.

## Build It

`ChartModel::range`, `view` and `text`, and `SignalChart::paintEvent` in `exercise/solution.hpp`.

```
rcpp verify 10-14
```

## Use It

`rc/qt/signal_chart.hpp` joins its module.

```cpp
rc::qt::SignalChart chart;
chart.model().set_window(0.0, 10.0);
chart.model().set_min_span(0.1);
chart.show();
chart.add_sample(t, value);                                   // from the timer or the queued signal of lesson 09-13
std::cout << chart.model().text(60, 15);                      // the same chart on a terminal
```

**What this does not do.** It draws one signal in one colour, with ticks and without labels in the window: text in a picture depends on fonts, which is why the window marks ticks and the terminal writes them. It is a model and two views and not an application: the zoom of lesson 10-09 and the pick of lesson 10-08 change the window and the view, and wiring them to the mouse is a few lines. It redraws all of its columns for each repaint (lesson 10-11 shows how to draw fewer), and the minimum span is a choice for each signal, not something it can know.

## What Breaks First

- **A range worked out twice.** See `E-QT-0063`.
- **A spike in two places.** See `E-QT-0064`.
- **Ticks that the views do not share.** See `E-QT-0065`.

## Ship It

`rc/qt/signal_chart.hpp` holds `ChartModel` and `SignalChart`.

This is what phase 10 was for: a live chart of any signal, in a window and in a terminal, that does not lie about a steady one. Phase 11 builds the interface in a different way, in a declarative language, and will want the same model.
