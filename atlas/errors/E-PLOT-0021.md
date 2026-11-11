id: E-PLOT-0021
title: A chart panned or zoomed into nothing
match: the window stops at the ends of the data, keeps its width when pushed, and cannot be narrower than the minimum
match: expected before.t0
platforms: linux, windows
teaches: 10-09-the-zoom-that-moved-the-point
---

## Symptom

Dragging past the first sample shows an empty chart, or zooming out shows years of nothing around the data, or zooming in far enough makes the axis labels 1e-15 apart.

## Cause

Nothing stops the window leaving the data unless something does. Measured: -10 to 15 became **0 to 25**, 50 to 75 became **35 to 60**, -100 to 400 became **0 to 60**, and a window 0.0002 wide became 1 wide.

## Fix

Clamp after every zoom and pan, and keep the width when pushing back:

```cpp
if (out.t0 < lo) { out.t0 = lo; out.t1 = lo + span; }
if (out.t1 > hi) { out.t1 = hi; out.t0 = hi - span; }
```
