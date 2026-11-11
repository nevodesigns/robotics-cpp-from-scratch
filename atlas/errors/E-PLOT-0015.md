id: E-PLOT-0015
title: Chart labels that are not on the rows they describe
match: every line is the same length, the labels are on tick rows and fall as the rows go down, and bad sizes give no lines
match: expected equal_length
platforms: linux, windows
teaches: 10-07-the-chart-in-a-terminal
---

## Symptom

The labels of a chart do not line up with its marks: 5.0 is written beside a row that is at 4.8, or the lines are different lengths and the chart is ragged.

## Cause

A label is only true on the row where its tick is, and the lines have to be padded to the widest label. Measured: 13 lines of the same length with **3** labels, each lower than the one above.

## Fix

Put each label on `row_for(tick)` and pad to the widest:

```cpp
labels[row_for(v, range, rows)] = tick_label(v, ticks.decimals);
out.push_back(std::string(width - l.size(), ' ') + l + " |" + grid[r]);
```
