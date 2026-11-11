id: E-PLOT-0010
title: A spike that a chart does not show
match: drawing every 1250th of a million samples never sees a spike that is not on one
match: expected naive_high
platforms: linux, windows
teaches: 10-06-a-million-points-in-800-columns
---

## Symptom

A chart of a long log looks clean and a fault that the engineer knows happened is nowhere on it. Zooming in finds it.

## Cause

Drawing every nth sample is a sample, and a one-sample spike is only in it if its index is a multiple of n. Measured: a spike of 5 in a million samples, every 1250th sample peaked at **0.001**; the columns of `decimate` had a highest value of **5**.

## Fix

Keep the lowest and highest value in each pixel column, and draw the line between them:

```cpp
if (s.value < col.low) col.low = s.value;
if (s.value > col.high) col.high = s.value;
```
