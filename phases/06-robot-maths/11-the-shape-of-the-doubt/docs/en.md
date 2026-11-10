# The Shape of the Doubt: Covariance, the Ellipse It Draws, and Why a Box Around a Position Is the Wrong Shape

> A position estimate is a point and a doubt about the point. The doubt has a shape, and the obvious way to write it
> down loses the tilt.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-12, 06-07

## The Problem

A localisation estimate says the robot is at (5, -3) give or take two metres in x and one in y. That is two numbers for the doubt, and it throws away a third: when x is high, is y likely to be high too? A robot drifting along a diagonal corridor is uncertain along the corridor and sure across it, and a box aligned to x and y cannot say so.

Filters that follow in phase 16 carry the doubt as a matrix, and decide whether to believe a measurement by how far it is from the prediction in units of that doubt. This lesson builds the matrix and the distance.

## The Concept

### Covariance

The **covariance** of points is three numbers: the variance of x, the variance of y, and the covariance of x with y, how much they move together. Each is a sum of products of distances from the mean, divided by one less than the number of points. Points (1, 2), (3, 6), (5, 10) give variances 4 and 16 and a covariance of 8.

20000 points made with spreads 2 and 1 and a correlation of 0.8 gave variances **3.98** and **0.98** (the true values are 4 and 1) and a covariance of **1.57** (true 1.60).

### The axes

A 2 by 2 symmetric matrix has its eigenvalues in closed form: the mean of the two variances plus or minus a radius, `sqrt(((xx - yy) / 2)^2 + xy^2)`. The long axis points half the angle of `(xx - yy, 2 xy)` from the x axis. The test cloud's long axis was at **23.2** degrees, with spreads 2.16 and 0.56.

A matrix of 9 and 1 on the diagonal has its long axis at 0 degrees, 1 and 9 at 90, and the matrix with 2, 1 and 2 at 45 degrees with eigenvalues 3 and 1.

### The ellipse and the box

A point's squared distance from the centre in units of the cloud's own spread is `(p - m)' C^-1 (p - m)`, the squared **Mahalanobis distance**; for a 2 by 2 matrix the inverse is written out. A point with a squared distance under **5.991** is inside the ellipse that holds 95 percent of a bell shaped cloud.

| region | share of the points | area |
|---|---|---|
| 95 percent ellipse | 95.2 percent | 22.3 |
| box of two spreads each way | 92.9 percent | 32.0 |

The box is bigger and holds less. See `E-MATH-0019`.

### Metres are not spreads

A point 2.2 metres out along the cloud had a squared distance of **1.00**. A point 2.00 metres out across it had **12.38**, far outside 5.991. A gate that accepts anything within a metre accepts the second and rejects the first. See `E-MATH-0020`.

### When there is no width

Points on a line have no spread across it, the determinant is zero, and the inverse divides by zero. The distance is **0** for the centre and **infinity** for any other point. See `E-MATH-0021`.

## Build It

`covariance`, `principal_axes` and `mahalanobis_squared` in `exercise/solution.hpp`. The point and matrix types, `mean_of`, `inside_ellipse` and the cloud generator are given.

```
rcpp verify 06-11
```

## Use It

`rc/math/covariance.hpp` joins `rc::math`. Its point type there is `Point2`.

```cpp
const Cov2 c = covariance(points);
if (!inside_ellipse(measurement, predicted, c, kInside95)) { /* reject it */ }
```

**What this does not do.** It is two dimensions. In three the closed form for the axes is a cubic and a library does it numerically; the matrix inverse is not written out by hand either. The 95 percent figure of 5.991 holds for a bell shaped cloud, and a cloud with long tails has more points outside. A sample covariance from a few points is itself uncertain: with 3 points it says very little. And the
covariance of the cloud of readings is the doubt of a measurement, not of a robot's position: the filter has to carry the second as time passes.

## What Breaks First

- **A box on a tilted cloud.** The wrong shape. See `E-MATH-0019`.
- **A gate in metres.** Too strict one way and too loose the other. See `E-MATH-0020`.
- **A covariance with no inverse.** Infinity or not a number. See `E-MATH-0021`.

## Ship It

`rc/math/covariance.hpp` holds `Cov2`, `mean_of`, `covariance`, `Axes`, `principal_axes`, `mahalanobis_squared`, `inside_ellipse`, `kInside95` and `correlated_cloud`.

Lesson 06-08 fitted a line by the direction of most spread. This gives that direction, and the spread across it, a name and a use.
