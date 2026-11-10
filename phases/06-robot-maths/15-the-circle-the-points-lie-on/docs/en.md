# The Circle the Points Lie On: Fitting a Circle to a Piece of an Arc, and the Bias a Quick Fit Has

> A robot drives a curve and wants to know its radius. The quick way to fit a circle is one linear solve, and on a short
> arc it is wrong by a lot.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-12, 06-07, 06-08

## The Problem

A robot has a turning radius, a wheel has a radius, and a lidar sees the curved side of a pipe. In each case there are points that lie, roughly, on a circle, and the job is to find its centre and radius.

Lesson 06-08 fitted a line. A circle has three numbers where a line has two, and the obvious way of fitting one has a bias that shows up exactly when the data are a short piece of the circle, which is the usual case for a robot.

## The Concept

### A circle as a linear problem

A point is on a circle with centre `(a, b)` and radius `r` when `(x - a)^2 + (y - b)^2 = r^2`. Expanding, `x^2 + y^2 + D x + E y + F = 0`, with `D = -2a`, `E = -2b` and `F = a^2 + b^2 - r^2`. That is **linear** in `D`, `E` and `F`. Making the left side as close to zero as possible for every point is a least squares problem with three unknowns, one 3 by 3 system of normal equations, which the solver of lesson 06-07 handles. This is the algebraic, or Kasa, fit.

Three points on a circle of radius 2 centred at (1, -1) gave exactly that circle. Exact points on a 120 degree arc gave a radius of 2 to 1e-8.

### What it does not minimise

The quick fit minimises the left side of the equation, which is not the distance of a point from the circle. It weights points by a quantity that grows with the circle's size, and on a short arc it pulls the answer toward a smaller circle. A circle of radius 2 with noise 0.02, 40 points, 300 runs, the mean error of the radius:

| arc | quick fit | refined |
|---|---|---|
| 360 degrees | -0.0002 | -0.0003 |
| 180 degrees | -0.0013 | -0.0006 |
| 90 degrees | -0.0212 | -0.0019 |
| 60 degrees | -0.1049 | -0.0047 |

All the way round the quick fit is fine. On 60 degrees it is five percent small. See `E-MATH-0031`.

### Refining

The real distance of a point from the circle is `rho - r`, where `rho` is its distance from the centre. Its derivative with respect to the centre is a unit vector pointing away from the point, and with respect to the radius it is -1. **Gauss-Newton** solves for the change that makes the sum of squared residuals smaller, again as a 3 by 3 system, and ten steps from the quick fit are enough. On an arc of 100 degrees the sum of squared distances went from 2.4582e-02 to 2.4395e-02.

### Some arcs say too little

Refining fixes the bias and cannot fix the information. The rms error of the radius with the refined fit: **0.006** for a 180 degree arc, **0.172** for 40 degrees and **5.986** for 15. On a gentle curve almost any large circle fits about as well, so the radius is not in the data. See `E-MATH-0032`.

### A line is not a circle

Ten points on a line gave a fit that was **not valid**: the circle through them is infinitely large. The function refuses, and the test also checks that a real arc is fitted, so a function that refused everything would not pass. See `E-MATH-0033`.

## Build It

`fit_circle_algebraic` and `refine_circle` in `exercise/solution.hpp`. The types and the arc generator are given.

```
rcpp verify 06-15
```

## Use It

`rc/math/fit_circle.hpp` joins `rc::math`. Its point type there is `CirclePoint`.

```cpp
Circle c = fit_circle_algebraic(points);
c = refine_circle(points, c, 10);
if (!c.valid) { /* say: no circle, or a line */ }
```

**What this does not do.** It treats every point alike, so a wrong point pulls it as it pulled the line fit of lesson 06-08: use the random-sample idea first if the points are not clean. It assumes the noise is the same on both coordinates and the same at every point, which a range sensor's is not (noise grows with distance). It can still return an enormous radius on a nearly straight set of points,
and a caller must look at the radius it gets. And it gives no uncertainty: lesson 06-11's covariance of the three numbers would be the next step.

## What Breaks First

- **The quick fit on a short arc.** The circle is too small. See `E-MATH-0031`.
- **An arc too short to say.** The radius scatters by metres. See `E-MATH-0032`.
- **Points on a line.** There is no circle. See `E-MATH-0033`.

## Ship It

`rc/math/fit_circle.hpp` holds `CirclePoint`, `Circle`, `fit_circle_algebraic`, `refine_circle` and `arc_points`.

Lesson 06-08 found the line the points lie on, and this finds the circle. Both are the same idea: write what the points should satisfy, and solve for the numbers that make it nearly so.
