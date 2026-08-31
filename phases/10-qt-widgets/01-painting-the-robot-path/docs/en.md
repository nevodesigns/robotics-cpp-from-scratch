# Painting: Watching the Robot You Wrote in Phase 00 Drive

> The arithmetic you wrote in your fourth lesson is about to draw itself on screen, unchanged.

**Type:** Build
**Time:** about 105 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04. Not claimed on Windows, because this repository's continuous integration does not build Qt there and an unproven claim is worth less than no claim.
**Qt:** 6.2 or newer, Widgets
**Hardware:** none, the robot is simulated
**Prerequisites:** 09-01, 00-04

## The Problem

You have been reading numbers. `x = 1.42, y = 0.31, theta = 0.78` is a true
description of where a robot is and a hopeless way to understand what it did.

Every roboticist eventually builds the same tool: a window that draws where the
machine went. It is the fastest debugging instrument in the field, because a
wrong turn is obvious in a picture and invisible in a column of numbers.

This lesson builds it, and it drives it with `rc::sim::step`, the function you
wrote in lesson 00-05, imported unchanged.

## The Concept

### Painting is a response, not a command

You never draw on a widget whenever you feel like it. Qt asks the widget to draw
itself, by calling `paintEvent`, when the window system needs the pixels: on
first show, on resize, on uncovering, or because you called `update()`.

```cpp
void PathView::paintEvent(QPaintEvent*) {
  QPainter painter(this);
  painter.fillRect(rect(), background_);
  // draw here, and only here
}
```

`update()` does not paint. It asks for a paint to happen soon, and several calls
before the next repaint collapse into one. That is a feature: a control loop
running at a kilohertz can call `update()` freely without producing a thousand
repaints a second.

Painting outside `paintEvent` either does nothing or produces flicker, and it is
the first thing to check when a custom widget misbehaves.

### Two coordinate systems, and the flip

The robot lives in metres, with y increasing upward, because that is how
mathematics and every robotics convention work.

The widget lives in pixels, with y increasing **downward**, because that is how
screens have worked since text terminals.

So drawing a path means mapping between them, and the mapping contains a
subtraction that everybody gets wrong once:

```cpp
const double px = margin + (pose.x - bounds.min_x) * scale;
const double py = height - margin - (pose.y - bounds.min_y) * scale;
//               ^^^^^^^^^^^^^^^^ the flip
```

Forget it and your robot drives a mirror image of its real path, which looks
plausible enough that you may not notice until it matters.

### Fitting the path to the window

The path could be a metre across or fifty. A view that only looks right at one
scale is not much use, so the widget computes the bounds of the path and picks a
scale that fits, taking the smaller of the horizontal and vertical fits so the
whole path is visible and the shape is not distorted.

A degenerate path, one point, or all points in a line, has zero width or height.
Dividing by that produces infinity, and everything downstream becomes NaN. Guard
it.

### Testing a widget without a screen

Most Qt tutorials never mention this, and it is what makes GUI code trustworthy.

A widget can render into an image with no window and no screen at all:

```cpp
QImage canvas(240, 180, QImage::Format_ARGB32);
canvas.fill(Qt::transparent);
view.render(&canvas);
```

Now the test can ask real questions of the result: were any pixels drawn, is the
background the colour it should be, does a path that runs off to the right put
its ink on the right hand side.

With `QT_QPA_PLATFORM=offscreen` this runs on a continuous integration machine
that has no display, which is why the tests for this lesson run in the same lane
as everything else.

## Build It

Implement in `exercise/solution.hpp`:

- `PathBounds bounds_of(const std::vector<rc::sim::Pose>& path)` returning the
  smallest and largest x and y. An empty path gives an empty bounds.
- `QPointF to_widget(const rc::sim::Pose& pose, const PathBounds& bounds, QSize size, double margin)`
  mapping a pose to a pixel position, fitting the bounds into the widget, keeping
  the aspect ratio, and flipping y. A degenerate bounds must not produce NaN.
- `PathView::paintEvent` filling the background and drawing the path as a
  connected line, with the final pose marked.

```
rcpp verify 10-01
```

The tests render the widget into an image and inspect the pixels.

## Use It

`QPainter` is the same interface used to draw onto a printer, an image, an OpenGL
surface or a PDF, so a view written this way can export a report without change.

For thousands of live points at high rates you would move to `QGraphicsView` or
Qt Quick's scene graph, which keep a structure of items rather than repainting
everything. For a path with a few thousand points repainted a few times a second,
which covers most robot tooling, this is the right tool and it is about forty
lines.

## What Breaks First

- **The program will not start, complaining about a platform plugin.** You are on
  a machine with no display. Run with `QT_QPA_PLATFORM=offscreen`. See
  `E-QT-0004`.
- **Nothing is drawn.** Painting was attempted outside `paintEvent`, or the
  painter was constructed on the wrong object. See `E-QT-0006`.
- **The path is upside down.** The y flip is missing, so the robot drives a
  mirror image of its real path. See `E-NUM-0004`.

## Ship It

`PathView` becomes the first component in `rc::qt`. Phase 12's control station
embeds it, phase 14 draws a controller's target next to its measured path in it,
and phase 16 draws a planned route through it. You will not write another one.
