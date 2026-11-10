id: E-MATH-0021
title: A covariance with no inverse
match: a cloud with no width in one direction has no distance for a point off its line
match: expected mahalanobis_squared
platforms: linux, windows
teaches: 06-11-the-shape-of-the-doubt
---

## Symptom

A gating step that works for every scan returns not a number, or accepts everything, the first time all the points happen to lie on a line: a robot facing a straight wall, or a single reading repeated.

## Cause

The distance needs the inverse of the covariance, which divides by its determinant. Points on a line have no spread across it, the determinant is zero, and the division gives infinity or not a number, which then fails or passes every comparison depending on the order it is written in.

Measured: ten points on a line. The distance of the centre was **0**, and of a point off the line **infinity**, which `inside_ellipse` rejects. With fewer than two points the covariance is all zeros.

## Fix

Check the determinant and say what the answer is when it is zero:

```cpp
if (det < 1e-18) return (dx == 0.0 && dy == 0.0) ? 0.0 : std::numeric_limits<double>::infinity();
```
