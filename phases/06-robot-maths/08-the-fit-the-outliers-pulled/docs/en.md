# The Fit the Outliers Pulled: Fitting a Line to Points With a Few Wrong Ones, and Lines That Go Straight Up

> A robot looks at a wall with a range sensor and has to say where the wall is. A chair in front of it, or a wall that
> runs straight ahead, is enough to ruin the usual way of doing that.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-12, 06-07

## The Problem

A range sensor returns points. Some are on a wall and some are not: a chair, a person, a reflection. The robot has to say where the wall is, as a line, so that it can drive along it.

Lesson 06-07 solved a system of equations and fitted a polynomial by least squares. A line is the first thing anyone fits, and it goes wrong in two ways that have nothing to do with the arithmetic: the wall may point straight at the sensor, and some points are not on the wall.

## The Concept

### A line that can go straight up

`y = m x + k` cannot describe a vertical line. Write it as `a x + b y + c = 0` with `a` and `b` a unit vector. Then `a x + b y + c` is the signed distance from the line to the point `(x, y)`, which is the thing a fit should make small, and `distance_to_line` is its absolute value.

Points on `x = 3` fitted by `fit_line` gave a valid line at **90.0** degrees. Fitted by vertical distances there was no answer. See `E-MATH-0010`.

### Perpendicular distances

Making the vertical distances small measures the error straight up, which for a steep line is far from the distance to the line. Making the perpendicular distances small has a closed form: the line passes through the centre of the points, and its direction is half of `atan2(2 Sxy, Sxx - Syy)`, where the S values are the spreads of the points about their centre.

A line at 85 degrees, 30 points with noise 0.1, 300 runs: the mean error of the angle was **0.77** degrees with vertical distances and **0.29** with perpendicular ones.

### An angle for a line

A line has no direction: `(a, b, c)` and `(-a, -b, -c)` are the same line and 1 degree and 181 are the same angle. `line_angle_degrees` gives an angle from 0 up to 180, and `angle_difference_degrees` the smaller way round: 179 and 1 are **2** apart. See `E-MATH-0012`.

### Wrong points

A least squares fit is pulled by a far point more than by a near one. 100 points on a line at 30 degrees with noise 0.02, plus 30 wrong points in a cluster, 200 runs:

| method | mean error of the angle |
|---|---|
| fitting everything | 20.69 degrees |
| random-sample fit | 0.03 degrees |

The random-sample fit (RANSAC) tries a hundred lines, each through two points chosen at random, counts the points within a threshold of each, keeps the line most points agree with, and then fits all the points that agreed. It kept all **100** true points close to the line in every run. See `E-MATH-0011`.

The seed is a parameter, so the same seed gives the same line.

## Build It

`fit_line` and `ransac_line` in `exercise/solution.hpp`. `Point`, `Line`, `distance_to_line`, the angle functions and the vertical distance fit, kept for comparison, are given.

```
rcpp verify 06-08
```

## Use It

`rc/math/fit_line.hpp` joins `rc::math`.

```cpp
const Line wall = ransac_line(points, 100, 0.05, seed);
if (wall.valid) { /* distance_to_line(wall, p) for any point */ }
```

**What this does not do.** It finds one line, and a room has four walls: find one, take its points away, and find the next. The threshold has to suit the sensor's noise, and a threshold that is too small keeps few points and one that is too large keeps the chair. With more than about half the points wrong it may fail, since the best line is then
not the wall. Its answer is random in the sense that another seed gives a slightly different line, though not by much. And it fits a straight line only: a curved wall is a different problem.

## What Breaks First

- **A slope and an intercept.** A wall straight ahead has no answer. See `E-MATH-0010`.
- **Fitting every point.** One chair pulls the line. See `E-MATH-0011`.
- **Comparing the numbers of a line.** The same line looks different. See `E-MATH-0012`.

## Ship It

`rc/math/fit_line.hpp` holds `Point`, `Line`, `distance_to_line`, `line_angle_degrees`, `angle_difference_degrees`, `fit_vertical_distances`, `fit_line` and `ransac_line`.

Lesson 06-07 fitted a curve to points that were all right. This fits one when some of them are not.
