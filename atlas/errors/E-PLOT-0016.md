id: E-PLOT-0016
title: A pixel mapped back to the wrong data
match: a point goes to a pixel and back to itself, and a higher value is a smaller y
match: expected off to equal 0
platforms: linux, windows
teaches: 10-08-the-click-beside-the-point
---

## Symptom

A hover readout is upside down: the cursor at the top of the chart reads the lowest value. Or it is a margin off, so every reading is wrong by the same amount.

## Cause

The map that drew the chart and the map that reads a click are written separately, and one of them forgets the margin or the flip. The y of a screen goes down while the value goes up. Measured: the lowest value was at y = **90** and the highest at y = **10**, and **25** points out and back were all within 1e-9.

## Fix

Write `from_pixel` as the algebraic inverse of `to_pixel` and test the round trip:

```cpp
d.value = v.range.low + (1.0 - (y - v.margin) / usable_down) * (v.range.high - v.range.low);
```
