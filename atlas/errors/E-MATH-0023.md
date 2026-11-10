id: E-MATH-0023
title: Two segments on one line that were said to touch
match: segments that cross, touch, overlap, miss and never meet
match: expected this to be true: !segments_intersect
platforms: linux, windows
teaches: 06-12-does-the-line-cross-the-wall
---

## Symptom

A planned path is declared blocked by a wall that is two metres away along the same line, or a path that really does run into a wall is declared clear. It only happens when the path and the wall are exactly in line, such as a corridor along one axis.

## Cause

The usual test, that the ends of each segment are on opposite sides of the other's line, fails when they are collinear: every orientation is zero and "opposite sides" means nothing. A fix that says "collinear counts as touching" is wrong for segments that are apart; one that says it does not count is wrong for overlapping ones.

The suite tried an X, a T, segments end to end, overlapping, one inside the other (all intersect), and then on one line with a gap between them, parallel, lines that cross where the segments do not reach, and a diagonal pair apart (none intersect).

## Fix

Handle the collinear cases by asking whether an end lies between the ends of the other:

```cpp
if (o1 == 0 && between(p1, p2, q1)) return true;
```
