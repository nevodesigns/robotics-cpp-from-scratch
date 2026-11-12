id: E-SAFE-0017
title: A distance to the line of an edge and not to the edge
match: the distance to an edge is to the segment, and past the end of a segment it is to the end
platforms: linux, windows
teaches: 19-06-the-line-it-may-not-cross
---

## Symptom

The robot is allowed too close to a corner of the fence, or a fence edge seems to be near a point that is nowhere near it.

## Cause

The point (15, 0) was 5 m from the segment (0,0) to (10,0), not 0, the distance to its line. In an L at (4, 4) the nearest edge was 1.414 m, the reflex corner, where the line of x = 5 would give 1.000.

## Fix

Clamp the projection to the segment.

```cpp
t = std::max(0.0, std::min(1.0, t));
```
