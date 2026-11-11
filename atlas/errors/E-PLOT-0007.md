id: E-PLOT-0007
title: Axis ticks made by adding a step, and labels that show the error
match: adding 0.1 ten times is not 1, and 3 times 0.1 prints as 0.30000000000000004
match: expected t.count to equal 6
platforms: linux, windows
teaches: 10-05-ticks-on-nice-numbers
---

## Symptom

An axis is labelled 0.30000000000000004, or its last tick is 0.9999999999999999 and falls off the end of the range, or the tick at 1 is missing.

## Cause

A step added to itself accumulates the rounding of every addition, and a double that is printed in full shows what it holds. Measured: 0.1 added ten times was **0.99999999999999989** and `3 * 0.1` printed as **0.30000000000000004**.

## Fix

Compute each tick from a whole number, and round it to the decimals of the step:

```cpp
std::round(static_cast<double>(first_index + k) * step * scale) / scale
```
