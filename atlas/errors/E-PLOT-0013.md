id: E-PLOT-0013
title: A steady signal drawn as violent noise
match: noise in the last digit of a steady signal fills the chart unless the range has a minimum span
match: expected rows_marked
platforms: linux, windows
teaches: 10-07-the-chart-in-a-terminal
---

## Symptom

A signal that is steady in every way that matters looks like a seismograph: the line swings from the top of the chart to the bottom, and an engineer chases a fault that is not there.

## Cause

A chart that scales its range to the data makes any span fill the screen, including the 2e-6 of noise in the last digit of a sensor. Measured: 5 plus or minus 1e-6 marked **11** of 13 rows scaled to the data and **1** with a minimum span of 1.

## Fix

Make the range no narrower than the smallest change that matters:

```cpp
return at_least(padded(range, 0.05), min_span);
```
