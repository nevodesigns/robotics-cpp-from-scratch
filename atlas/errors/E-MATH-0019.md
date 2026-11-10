id: E-MATH-0019
title: A box drawn around a position that has a tilted uncertainty
match: the ellipse holds the share of points it should, and the box does not match it
match: expected this to be true: box_area
platforms: linux, windows
teaches: 06-11-the-shape-of-the-doubt
---

## Symptom

A robot's position estimate is said to be good to two spreads in x and two in y, and a safety check uses that box. The check keeps rejecting positions that are fine, and accepting ones that are not, and the clearance it leaves around obstacles is bigger than the robot needs.

## Cause

When x and y move together, the cloud of possible positions is a tilted ellipse, and a box that is aligned to the axes does not fit it. It leaves out the corners where the cloud is dense and takes in corners where it is empty.

Measured: 20000 points from a cloud with spreads 2 and 1 and a correlation of 0.8. The 95 percent ellipse held **95.2** percent of the points and had an area of **22.3**. A box of two spreads each way held **92.9** percent and had an area of **32.0**, more than the ellipse with fewer points in it.

## Fix

Use the ellipse the covariance describes: points within a squared Mahalanobis distance of 5.991 of the centre hold 95 percent of a bell shaped cloud in two dimensions:

```cpp
inside_ellipse(p, centre, cov, kInside95);
```
