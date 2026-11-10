id: E-MATH-0033
title: A circle fitted to points that lie on a line
match: points on a straight line are not on any circle, and the fit says so
match: expected this to be true: !c.valid
platforms: linux, windows
teaches: 06-15-the-circle-the-points-lie-on
---

## Symptom

A circle fit applied to the points of a straight wall returns a circle of radius ten to the twelfth, or not a number, and the next stage multiplies by it.

## Cause

Points on a line have no circle through them in the ordinary sense: the circle through them is infinitely large. The fit's matrix is then singular or the radius squared comes out as a rounding artefact.

Measured: ten points on a line gave a fit that was **not valid**. The same function fitted a 270 degree arc of a circle of radius 1 as valid. A test of that case needs a positive check as well as the refusal, or a function that refuses everything would pass.

## Fix

Return an invalid circle when the solve fails or the radius squared is not above zero, and let the caller decide what a line means:

```cpp
if (!ok) return c;
if (!(r2 > 0.0)) return c;
```
