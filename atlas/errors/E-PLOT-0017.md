id: E-PLOT-0017
title: The nearest sample in seconds and volts
match: the sample nearest a click in pixels is not the nearest in seconds and volts
match: expected by_pixels to equal 1
platforms: linux, windows
teaches: 10-08-the-click-beside-the-point
---

## Symptom

A hover picks a sample that is nowhere near the cursor. It is right on a chart whose axes are the same size and wrong on a real one.

## Cause

A distance of the form `sqrt(dt*dt + dv*dv)` adds seconds to volts. Which is nearer depends on the units. Measured: in raw units A was **0.4** away and B **0.9**; in pixels A was **40** and B **9**, and B was under the cursor.

## Fix

Measure in pixels, after mapping each sample to the screen:

```cpp
const PixelPoint p = to_pixel(v, s.time, s.value);
const double distance = (p.x - x) * (p.x - x) + (p.y - y) * (p.y - y);
```
