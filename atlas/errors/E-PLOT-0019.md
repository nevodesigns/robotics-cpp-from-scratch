id: E-PLOT-0019
title: A zoom that moves the point under the cursor
match: zooming about the middle throws the point under the cursor off the screen, and zooming about it leaves it put
match: expected x_of(about, anchor)
platforms: linux, windows
teaches: 10-09-the-zoom-that-moved-the-point
---

## Symptom

Scrolling the wheel on a spike zooms toward the middle of the chart, and the spike slides away from the cursor and off the screen.

## Cause

A zoom that scales the window about its centre keeps the centre fixed, not the point the person is looking at. Measured: the point at time 80 of 0 to 100 on 1000 pixels was at x = **800**; after zooming by 2 about the middle it was at **1100**, and about the cursor it stayed at **800**.

## Fix

Keep the anchor at the same fraction of the window:

```cpp
const double fraction = (anchor - w.t0) / span;
out.t0 = anchor - fraction * new_span;
```
