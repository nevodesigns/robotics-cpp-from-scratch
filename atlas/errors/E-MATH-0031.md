id: E-MATH-0031
title: A circle fitted by the quick method, too small on a short arc
match: on a short arc the quick fit makes the circle too small, and refining removes most of it
match: expected this to be true: quick_bias_60
platforms: linux, windows
teaches: 06-15-the-circle-the-points-lie-on
---

## Symptom

A robot drives an arc to calibrate its turning radius, and the fitted radius is a few percent too small. A longer arc, all the way round, gives the right answer, so the fit looks fine on the bench and wrong in the corridor.

## Cause

The quick fit makes `x^2 + y^2 + D x + E y + F` small for every point. That is linear in `D`, `E` and `F`, so a single solve gives the circle, but it is not the distance of a point from the circle. It weights points by a quantity that grows with the size of the circle, and on a short arc this pulls the answer toward a smaller circle.

Measured, for a circle of radius 2 with noise 0.02 over 300 runs, the mean error of the radius for the quick fit and for the refined fit:

| arc | quick | refined |
|---|---|---|
| 360 degrees | -0.0002 | -0.0003 |
| 180 | -0.0013 | -0.0006 |
| 90 | -0.0212 | -0.0019 |
| 60 | **-0.1049** | **-0.0047** |

## Fix

Start from the quick fit and refine by Gauss-Newton on the real distances, whose residual is `rho - r` with derivatives `-(x - cx) / rho`, `-(y - cy) / rho` and `-1`:

```cpp
const Circle better = refine_circle(pts, quick, 10);
```
