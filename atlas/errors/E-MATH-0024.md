id: E-MATH-0024
title: A point whose ray goes through a corner, counted twice
match: a point in a polygon, with the ray going exactly through corners
match: expected correct_wrong
platforms: linux, windows
teaches: 06-12-does-the-line-cross-the-wall
---

## Symptom

A point inside a polygon is reported as outside, but only for some positions: those whose height is exactly the height of a corner. A polygon rotated by a fraction of a degree makes the failures disappear.

## Cause

Ray casting counts the edges a ray from the point crosses. A ray through a corner crosses two edges at once and is counted twice, which flips the answer back.

Measured: a diamond with its corners on the axes and 625 grid points 0.25 apart. The test that counts every edge whose height range includes the ray, ends included, was wrong for **17** points. The half-open test was wrong for **0**. It counts an edge only if one end is strictly above the ray and the other is not, so a corner on the ray belongs to one of the two edges that meet there.

## Fix

```cpp
if ((a.y > p.y) != (b.y > p.y)) { /* count this edge */ }
```

A point on an edge or a corner is tested separately and counts as inside.
