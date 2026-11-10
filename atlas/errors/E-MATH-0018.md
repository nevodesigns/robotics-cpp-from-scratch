id: E-MATH-0018
title: Choosing between a method that is fast and one that cannot fail
match: Newton with a bracket takes Newton's steps while they help, and always gets there
match: expected this to be true: cos_root
platforms: linux, windows
teaches: 06-10-where-it-crosses-zero
---

## Symptom

A solver uses halving because Newton once failed, and runs ten times slower than it needs to in a loop at a kilohertz. Or it uses Newton because it is fast and fails at three in the morning on an input that was never tried.

## Cause

Halving is slow and sure. Newton is fast and not sure. Keeping the root inside a bracket and taking Newton's step only when it lands inside the bracket and at least halves the value of f gives both: the bracket shrinks on every step either way.

Measured, steps to 1e-12: on cos(x) - x halving took **40**, Newton alone **4**, with a bracket **4**. On the arctangent over [-3.3, 5.1] halving took **43**, Newton alone failed, with a bracket **5**. On the cube root over [-1, 2] halving took **32**, Newton alone failed, and with a bracket it took **32**: every Newton step was refused, and it was halving throughout. With no sign change, it declined to promise.

## Fix

```cpp
use_newton = next > a && next < b;
if (use_newton && std::fabs(f(next)) > 0.5 * std::fabs(fx)) use_newton = false;
if (!use_newton) next = 0.5 * (a + b);
```
