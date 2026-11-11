id: E-PLOT-0012
title: A chart that costs as many points as it has samples
match: a million samples are drawn with at most two points a column, and a flat column is one point
match: expected this to be true: points
platforms: linux, windows
teaches: 10-06-a-million-points-in-800-columns
---

## Symptom

The chart is smooth with a thousand samples and takes seconds to redraw with a million, and the frame rate falls as the log grows.

## Cause

Drawing a line for every sample is work in proportion to the samples, and the screen has only as many columns as it has pixels. Measured: a million samples were drawn as **1600** points, two for each of 800 columns.

## Fix

Draw the columns and not the samples:

```cpp
points += c.low == c.high ? 1 : 2;     // never more than two for a column
```
