id: E-PLOT-0011
title: The newest sample, or the boundary sample, missing from a decimated chart
match: every sample is in exactly one column, and the output has one column per pixel
match: a sample on a boundary goes to the column on its right, and samples outside the range are not counted
platforms: linux, windows
teaches: 10-06-a-million-points-in-800-columns
---

## Symptom

A decimated chart is missing its newest sample, or its sample count is one short, or a column index is past the end and the program writes outside a vector.

## Cause

A bin is half open, so the sample at exactly `t1` computes a column number equal to the number of columns. Measured: with the clamp, **1000000** of a million samples were counted and the last column took the one at `t_end`; without it that sample is dropped or written out of range.

## Fix

Clamp the column number to the last column:

```cpp
int c = static_cast<int>((s.time - t0) / width);
if (c >= columns) c = columns - 1;
```
