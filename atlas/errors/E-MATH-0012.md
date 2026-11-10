id: E-MATH-0012
title: Two descriptions of one line that compare as different
match: the distance from a point to a line is perpendicular and the angle ignores which way the line points
match: expected line_angle_degrees
platforms: linux, windows
teaches: 06-08-the-fit-the-outliers-pulled
---

## Symptom

A test compares the fitted line with the expected one and fails because the angle came out as 179 degrees and 1 was expected, or the signs of every coefficient are the opposite. Both describe the same line.

## Cause

A line has no direction. `a x + b y + c = 0` and `-a x - b y - c = 0` are the same set of points, and an angle of 1 degree and one of 181 are the same line. Comparing the numbers as they are compares the way the line was written and not the line.

The suite checked that a line and its flipped description both have an angle of **0.0**, that a vertical line is **90.0**, and that **179** and **1** are **2** degrees apart.

## Fix

Take the angle of the line modulo 180 degrees, and compare angles by the smaller way round:

```cpp
if (d > 90.0) d = 180.0 - d;
```
