id: E-SAFE-0016
title: A point in polygon test that fails exactly at a vertex
match: the first version of the test, which counts an end on the ray twice, calls an inside point outside
match: points are inside or outside a square and an L, and a point on the boundary is not inside
platforms: linux, windows
teaches: 19-06-the-line-it-may-not-cross
---

## Symptom

The fence works on every test point, and a robot in the middle of a field laid out on a grid is told it is outside.

## Cause

For the diamond (2,0), (4,2), (2,4), (0,2) and the point (1, 2), at the height of the right corner, a count that included both ends of an edge said outside, and the half open rule (a.y > p.y) != (b.y > p.y) said inside. A point on an edge or a vertex of a square was not inside.

## Fix

Count an edge only when exactly one end is above the point.

```cpp
if ((a.y > p.y) != (b.y > p.y)) { /* the edge crosses the ray's height */ }
```
