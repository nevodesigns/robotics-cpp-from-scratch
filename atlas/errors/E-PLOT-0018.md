id: E-PLOT-0018
title: A readout for a sample that is not on the chart
match: samples off the chart cannot be picked, and a view with no size does not divide by zero
match: expected pick
platforms: linux, windows
teaches: 10-08-the-click-beside-the-point
---

## Symptom

Hovering at the edge of a chart shows the value of a sample from before the window, or the program prints nan when the chart is first opened, before it has a size.

## Cause

A sample outside the time shown is still in the list, and its pixel is off the chart, but a click can be there too. And a view whose usable size is zero divides by it. Measured: samples at -5 s and 105 s were at x = -50 and 1050 and were **not** picked.

## Fix

Skip samples outside the view, and check the size before dividing:

```cpp
if (s.time < v.t0 || s.time > v.t1) continue;
if (usable_across > 1e-12) d.time = ...;
```
