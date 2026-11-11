# The Zoom That Moved the Point: Zooming About the Cursor, Panning in Pixels and Stopping at the Data

> Zoom about the cursor, so the point under it stays put. Make a turn out the exact inverse of a turn in. Stop at the ends of the data.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 10-08, 10-02

## The Problem

A person works a chart with the wheel and the mouse: zoom in on a spike, drag to look left, stop at the end. Three things feel wrong when they are slightly off, and each is a few lines of arithmetic that is easy to get nearly right.

## The Concept

### About the cursor

A chart of 0 to 100 seconds over 1000 pixels, with the cursor on time 80 at x = **800**. Zooming by 2 about the middle moved that point to x = **1100**, off the right edge. Zooming about the cursor left it at **800**. See `E-PLOT-0019`.

`zoom_about` keeps the anchor at the same fraction of the window. **216** zooms, for three starts, three spans, four anchor positions from the left edge to the right edge, and six factors from 0.1 to 7.5: **0** moved the anchor and **0** had the wrong width. A factor that is not positive changes nothing.

### Out is the inverse of in

A wheel step of 1.12 in and 0.88 out is 0.9856, not 1. Five notches in and five out, from a width of 100, left the window **107.522** wide. `wheel_factor` is 1.2 to the power of the notches, with 120 as one notch (as Qt reports it): five in and five out left **0 to 100** exactly, and a half notch, as a trackpad reports, is the square root of 1.2. See `E-PLOT-0020`.

### Dragging

A drag of 100 pixels to the right on 1000 pixels of 0 to 100 gave **-10 to 90**: the window moves the other way, and the point that was at x = 500 is at x = **600**, with the hand. A width of zero changes nothing.

### The ends

With data from 0 to 60 and a minimum width of 1: a window of -10 to 15 became **0 to 25**; 50 to 75 became **35 to 60**, the width kept; -100 to 400 became **0 to 60**; and a window 0.0002 wide became 1 wide, centred where it was. Data narrower than the minimum is shown whole. See `E-PLOT-0021`.

## Build It

`zoom_about`, `pan_by_pixels`, `wheel_factor` and `clamp_to` in `exercise/solution.hpp`.

```
rcpp verify 10-09
```

## Use It

`rc/plot/zoom.hpp` joins its module.

```cpp
window = rc::plot::zoom_about(window, cursor_time, rc::plot::wheel_factor(event_angle_delta));
window = rc::plot::clamp_to(window, data_first, data_last, 0.01);
```

**What this does not do.** It zooms the time axis only: the value axis is the range of lesson 10-07 and follows the data. It does not animate, so a zoom is one jump, and it does not decide what the minimum width is: a chart of microseconds and one of hours want different ones. And it does no inertia: a drag that ends is a drag that ends.

## What Breaks First

- **A zoom about the wrong point.** See `E-PLOT-0019`.
- **A wheel that does not come back.** See `E-PLOT-0020`.
- **A pan into nothing.** See `E-PLOT-0021`.

## Ship It

`rc/plot/zoom.hpp` holds `Window`, `zoom_about`, `pan_by_pixels`, `wheel_factor` and `clamp_to`.

Lesson 10-08 turned a click into data, and this changes what the click is a click on.
