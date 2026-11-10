id: E-MATH-0011
title: A fit pulled off by points that are not on the line
match: thirty wrong points among a hundred pull an ordinary fit far off, and a random-sample fit ignores them
match: expected this to be true: sum_all
platforms: linux, windows
teaches: 06-08-the-fit-the-outliers-pulled
---

## Symptom

The wall is straight and the fit of it is crooked, because a chair stood in front of part of it. The fit lies somewhere between the wall and the chair, and is right about neither.

## Cause

A least squares fit minimises the sum of squared distances, and a point far from the line costs far more than one near it. Thirty points in a cluster away from the line pull the whole fit toward them.

Measured: 100 points on a line at 30 degrees with noise 0.02, plus 30 wrong points in a cluster, 200 runs. Fitting everything gave a mean error of the angle of **20.69** degrees. The random-sample fit gave **0.03**, and kept all **100** true points close to the line in every run.

## Fix

Fit lines through pairs of points chosen at random and keep the one that most points agree with, then refit using only the points that agree:

```cpp
if (inliers.size() > best_inliers.size()) best_inliers = inliers;
...
return fit_line(best_inliers);      // all that agreed, not just the two that chose it
```
