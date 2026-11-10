id: E-MATH-0022
title: Three points in a line that a cross product says are not
match: which side of a line a point is on, and a line three decimal numbers cannot quite draw
match: expected this to be true: raw
platforms: linux, windows
teaches: 06-12-does-the-line-cross-the-wall
---

## Symptom

A robot is on the wall's line, on paper, and the check "is it on the line" says no, sometimes, depending on where the wall is. A collision test fails for a wall at 0.1 and passes for a wall at 0.5.

## Cause

`0.1` and `0.3` are not exactly representable in binary, so points that are on a line on paper are very slightly off it once stored. The cross product, which is zero exactly when three points are in a line, comes out as a tiny number of either sign.

Measured: `(0.1, 0.3)`, `(0.7, 2.1)` and `(1.1, 3.3)` lie on y = 3x. `cross()` gave **-2.22e-16**. `orientation()` compares it with the size of the triangle's sides, and said **0**. A point 1e-6 off a line of length 1 was still correctly seen as on one side.

## Fix

Compare with a tolerance that scales with the sizes involved:

```cpp
if (std::fabs(v) <= 1e-12 * scale) return 0;       // scale: the lengths of the two sides
```
