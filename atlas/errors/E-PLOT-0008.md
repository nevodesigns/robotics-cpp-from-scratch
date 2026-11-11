id: E-PLOT-0008
title: An axis that is labelled minus zero
match: a label has the decimals of the axis, and a value that rounds to zero is not written minus zero
match: expected tick_label
platforms: linux, windows
teaches: 10-05-ticks-on-nice-numbers
---

## Symptom

The middle of an axis that crosses zero is labelled -0.0 or -0.00, and the tick above it is 0.1.

## Cause

A small negative number printed with fixed decimals keeps its sign: `-0.004` with two decimals is **-0.00**. It is a valid way to print a number that is below zero and not a way an axis should say zero.

## Fix

After formatting, if every character is a zero, a point or a minus, take the minus off:

```cpp
if (all_zero && text[0] == '-') text.erase(0, 1);
```

`-0.006` is still `-0.01`.
