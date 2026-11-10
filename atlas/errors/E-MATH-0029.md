id: E-MATH-0029
title: A total in single precision, a percent out
match: a million tenths in single precision: the plain total is a percent out, and Kahan's is right to the digits a float has
match: expected this to be true: std::fabs
platforms: linux, windows
teaches: 06-14-adding-a-long-list
---

## Symptom

A counter or an average in `float` is a percent or more wrong after a million samples, and it gets worse the longer the program runs. The same code in `double` looks fine, for a while.

## Cause

A `float` keeps about seven digits, so the same effect starts at a much smaller total: once the sum is a hundred thousand, adding 0.1 loses a large part of it each time.

Measured: a million copies of the float 0.1. The exact total is **100000.00**. The plain total was **100958.34**, **0.96** percent out. Kahan's total was **100000.00**.

## Fix

Use `double` for an accumulator even when the readings are `float`, or add with compensation:

```cpp
float lost = (next - total) - corrected;
```
