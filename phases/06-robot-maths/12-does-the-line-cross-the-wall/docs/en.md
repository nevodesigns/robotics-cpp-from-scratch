# Does the Line Cross the Wall: Segments, Polygons, and the Special Cases That Break the Obvious Test

> Is the planned path blocked by a wall? Is the robot inside the keep-out zone? Each is a short piece of geometry,
> and each has a case where the first version you write gives the wrong answer.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 06-01, 00-04

## The Problem

A robot's world is walls and zones: a segment is a wall, a polygon is a keep-out region or a room. The questions it asks are simple. Does this path cross that wall? Where on the wall is it nearest to me? Is this point inside the zone? What is the zone's area?

The first version of each is a few lines, and each has an input where it fails: points in a line, segments that lie on the same line, a point at the height of a corner. All of them are common in a world built from axis aligned walls.

## The Concept

### Which side of a line

The cross product of two vectors is positive if the second is to the left of the first, negative if to the right, and zero if they are in a line. `orientation` gives 1, -1 or 0.

It cannot be tested as `cross == 0`. Three points on `y = 3x`, `(0.1, 0.3)`, `(0.7, 2.1)` and `(1.1, 3.3)`, are in a line on paper. Stored as doubles, `cross` gave **-2.22e-16**, and `orientation`, which compares it with the sizes of the triangle's sides, said **0**. A point 1e-6 off a line of length 1 was still on one side. See `E-MATH-0022`.

### Do two segments touch

If the ends of each are on opposite sides of the other's line, they cross. The rest are cases: an end of one that lies on the other (a T, or end to end), segments on one line that overlap or lie one inside the other, and the case a quick test gets wrong, segments on one line with a gap between them.

The suite checked an X, a T, end to end, overlapping and one inside the other (all intersect); on one line with a gap, parallel, lines that cross where the segments do not reach, and a diagonal pair apart (none do). See `E-MATH-0023`.

### The nearest point on a segment

Project the point onto the segment's line, hold the result between the ends, and that is the nearest point. A point 4 along and 3 across was **3** from the segment; one 13 along and 4 across was **5** from it, since the nearest point is the end. A segment of no length is a point.

### Inside a polygon

Cast a ray to the right and count the edges it crosses. An odd number means inside. A ray that passes exactly through a corner crosses two edges and is counted twice. The half-open rule counts an edge only if one end is strictly above the ray and the other is not, so a corner on the ray belongs to one edge.

A diamond with corners on the axes and 625 grid points 0.25 apart: the obvious test was wrong for **17**, the half-open one for **0**. A point on an edge or corner counts as inside. See `E-MATH-0024`.

### The area

Half the sum over the edges of `xj * yi - xi * yj` is the area, positive if the corners go anticlockwise and negative if clockwise. A 4 by 3 rectangle was 12, and -12 reversed; the diamond was 2; an L of a 4 by 4 square with a 2 by 2 corner cut out was 12.

## Build It

`orientation`, `segments_intersect`, `closest_point_on_segment`, `point_in_polygon` and `polygon_area` in `exercise/solution.hpp`. The point type, `cross`, `between` and `distance_to_segment` are given.

```
rcpp verify 06-12
```

## Use It

`rc/math/geometry.hpp` joins `rc::math`. Its point type there is `GeoPoint`.

```cpp
if (segments_intersect(from, to, wall_a, wall_b)) { /* the path is blocked */ }
if (point_in_polygon(robot, keep_out)) { /* stop */ }
```

**What this does not do.** The tolerance of 1e-12 of the lengths is a choice, and a world with coordinates in millimetres and walls kilometres long needs thought about it. It tests a point in a polygon but not a polygon in a polygon, and a path that passes through a zone without a corner in it is found only by testing the path's segments against
the zone's edges. It does not handle polygons that cross themselves, whose inside is a matter of definition. And a robot is not a point: to know whether it fits, grow the walls by its radius first.

## What Breaks First

- **A cross product compared with zero.** Points in a line are not. See `E-MATH-0022`.
- **Collinear segments.** Apart or overlapping, both look alike. See `E-MATH-0023`.
- **A ray through a corner.** Counted twice. See `E-MATH-0024`.

## Ship It

`rc/math/geometry.hpp` holds `GeoPoint`, `cross`, `orientation`, `between`, `segments_intersect`, `closest_point_on_segment`, `distance_to_segment`, `point_in_polygon` and `polygon_area`.

Lesson 06-01 introduced the two products of vectors. This is the cross product at work, and the same arithmetic as the area of a triangle.
