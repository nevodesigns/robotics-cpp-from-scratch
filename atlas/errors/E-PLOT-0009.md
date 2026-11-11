id: E-PLOT-0009
title: No ticks, or a division by zero, for a steady signal
match: a steady signal has one tick, at its own value, and a nearly steady one has a step that is not zero
match: expected flat.count to equal 1
platforms: linux, windows
teaches: 10-05-ticks-on-nice-numbers
---

## Symptom

A chart of a constant signal has an axis with no numbers, or the program divides by zero and prints nan, or loops over a step that is zero.

## Cause

The step is the span over the target, and a steady signal has a span of zero. Even a span of 1e-14 at a value of 5 is below what a double can tell apart, so any step made from it is noise. Measured: 5 to 5 and 5 to 5 + 1e-14 each had **1** tick, and 5 to 5 + 1e-6 had a real step.

## Fix

Say that the range is one value when its span is not more than 1e-12 times the biggest magnitude, and give it one tick at that value:

```cpp
if (!(span > 1e-12 * std::max(1.0, size))) { t.steady = true; t.single = low; t.count = 1; return t; }
```
