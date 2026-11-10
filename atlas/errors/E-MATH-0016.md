id: E-MATH-0016
title: Newton's method that ran away from the root
match: Newton can run away from the root, and says so
match: expected this to be true: !cube.converged
platforms: linux, windows
teaches: 06-10-where-it-crosses-zero
---

## Symptom

A solver that finds a joint angle, or the time a trajectory reaches a distance, works for most inputs and returns an enormous number, or not a number, for a few. The few are not obviously different.

## Cause

Newton's method goes to where the tangent line crosses zero. Where the slope is small the tangent crosses far away, and for some functions the crossing is always further from the root than the point it started from.

Measured: the cube root from 1 doubled its distance from zero every step and reached **1.1e+12** in 40 steps. The arctangent from 2, whose tangent is nearly flat out there, was at **-2.3e+10** after 5 steps. `x^2 - 2` from zero has a slope of exactly zero. All three were reported as not converged.

## Fix

Stop when the value runs away or the slope is zero, and report failure instead of an answer:

```cpp
if (std::fabs(slope) < 1e-14) return failure;
if (!(std::fabs(x) < 1e12)) return failure;
```

Better, keep the root bracketed and take Newton's steps only while they help (the next entry).
