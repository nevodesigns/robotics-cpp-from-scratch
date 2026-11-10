id: E-MATH-0030
title: A total checked against another total that is also rounded
match: four million numbers: the plain total is wrong in the fourteenth digit, and the other two are right to the last one
match: expected this to be true: e_naive
platforms: linux, windows
teaches: 06-14-adding-a-long-list
---

## Symptom

A test says a summation is accurate because it agrees with another summation of the same list. Both lose the same pieces, and the test passes for a total that is wrong.

## Cause

The reference has to be exact, or at least much more exact than what it checks. A second plain sum, or a sum in `long double`, which on some platforms is only a `double`, is not a reference.

The suite makes numbers that are whole multiples of 2^-40, so each is a whole number of units and the exact total is the sum of integers, which a 64 bit integer holds without rounding for 4 million numbers. Measured against that, over 4 million numbers with an exact total of 1999858.801753: the plain total had a relative error of **2.53e-14**, and pairwise and Kahan **0**.

## Fix

Build test data with an exactly known total, such as integers scaled by a power of two:

```cpp
total += k;                                   // exact, in a 64 bit integer
values.push_back(std::ldexp(static_cast<double>(k), -40));
const double exact = std::ldexp(static_cast<double>(total), -40);
```
