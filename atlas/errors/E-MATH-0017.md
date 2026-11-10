id: E-MATH-0017
title: Halving started where there may be no crossing
match: halving finds the crossing, in about forty steps for an interval of one and a tolerance of 1e-12
match: expected r.iterations
platforms: linux, windows
teaches: 06-10-where-it-crosses-zero
---

## Symptom

A root finder is given an interval and returns a number that is not a root, because the function does not cross zero in it. Nothing reports it, and the number is used.

## Cause

Halving is guaranteed to find a crossing only if the function has opposite signs at the two ends. With the same sign at both ends there may be no crossing, or an even number of them, and the method has nothing to follow.

Measured: `bisect` on cos(x) - x over [0, 1] to 1e-12 took **40** steps, as halving an interval of 1 down to 1e-12 must (log2 of 10^12 is about 40). With both ends of `x - 3` above zero (4 and 5) it reported **not converged**, an end that was exactly the root was returned at once, and a wide interval [-10, 100] still found 3.

## Fix

Check the signs first and refuse otherwise:

```cpp
if ((fa > 0.0) == (fb > 0.0)) return r;        // converged stays false
```
