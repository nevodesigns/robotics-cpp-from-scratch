id: E-PLOT-0014
title: A value that falls outside the chart
match: the range has five percent of room, and is never narrower than the minimum span
match: expected row_for
platforms: linux, windows
teaches: 10-07-the-chart-in-a-terminal
---

## Symptom

A chart draws nothing for the highest or lowest samples, or writes outside its grid and corrupts memory, or the extremes sit on the frame and cannot be told from it.

## Cause

A row computed from a value outside the range is outside the grid. And without room around the data, the highest sample is on the top row, touching the frame. Measured: 99 on a range of 0 to 10 is row 0 and -99 the last row, and 1 to 3 became **0.9 to 3.1**.

## Fix

Keep the row inside the chart, and pad the range:

```cpp
if (row < 0) row = 0;
if (row > rows - 1) row = rows - 1;
```
