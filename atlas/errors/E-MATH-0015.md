id: E-MATH-0015
title: A curve that divides by a zero time when a waypoint repeats
match: waypoints on a straight line stay on it, and repeated waypoints are taken once
match: expected worst to be within
platforms: linux, windows
teaches: 06-09-a-path-through-the-waypoints
---

## Symptom

A path with a repeated waypoint, which a planner produces when a robot is already at the next goal, comes back full of not-a-number values. The robot's next position is garbage and it stops.

## Cause

The curve is timed by the distances between waypoints, raised to a power. Two waypoints in the same place give a distance of zero, a time interval of zero, and the blending divides by it.

The sampler takes repeated waypoints once, and the curve function goes straight when any interval is under 1e-12. Measured: waypoints on a line with the last one repeated stayed exactly on it (a worst deviation of 0) for all three values of `alpha`, with a sharpest turn of 0 and a length of **10**, and 5 segments of 8 samples plus the end made **41** points. The curve has no neighbour before the first waypoint or after the last, so it makes one by reflecting the next waypoint through the end.

## Fix

```cpp
if (w.empty() || length(p - w.back()) > 1e-12) w.push_back(p);
const P p0 = i > 0 ? w[i - 1] : w[0] * 2.0 - w[1];
```
