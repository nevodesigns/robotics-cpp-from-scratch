id: E-MATH-0010
title: A line fitted with slope and intercept, and a wall that goes straight up
match: exact points give the exact line, including a line that goes straight up
match: expected this to be true: u.valid
platforms: linux, windows
teaches: 06-08-the-fit-the-outliers-pulled
---

## Symptom

A robot fits a line to the points of a wall from its range sensor. Facing a wall that runs straight ahead of it, the fit returns nonsense or not a number, and a wall at a steep angle comes out wobbling by several degrees from one scan to the next.

## Cause

`y = m x + k` has no way to describe a vertical line: its slope is infinite, and the sums used to find it divide by zero. A near vertical line has an enormous slope, and a small noise in x changes it by a lot. Fitting also measures the error straight up, which for a steep line is far from the distance to the line.

Measured: points on x = 3. `fit_line` gave a valid line at **90.0** degrees. The vertical distance fit was **not valid**. For a line at 85 degrees with 30 points and noise 0.1 over 300 runs the mean error of the angle was **0.77** degrees with vertical distances and **0.29** with perpendicular ones.

## Fix

Write the line as `a x + b y + c = 0` with `a` and `b` a unit vector, and make the perpendicular distances small. The direction is half of `atan2(2 Sxy, Sxx - Syy)`:

```cpp
const double theta = 0.5 * std::atan2(2.0 * sxy, sxx - syy);
line.a = -std::sin(theta);  line.b = std::cos(theta);
```
