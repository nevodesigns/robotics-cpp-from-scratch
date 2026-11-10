id: E-MATH-0013
title: A path of straight lines that a robot cannot follow
match: straight lines between waypoints turn through a full corner, and the curve does not
match: expected this to be true: curve_turn
platforms: linux, windows
teaches: 06-09-a-path-through-the-waypoints
---

## Symptom

A robot told to visit four waypoints stops at each, turns on the spot, and sets off again. Asked to keep moving it overshoots every corner or has to slow to a crawl at each.

## Cause

Between waypoints the path is a straight line, so at each waypoint the direction changes by the whole angle in a single step. No real machine can change its heading instantly, and a controller that tries to follow demands an unbounded turn rate.

Measured on a square route: the straight lines turned **90.0** degrees at a corner in one step. A curve through the same four waypoints, 20 points to a segment, turned at most **10.9** degrees in any step, and was 30.93 long against 30.00.

## Fix

Pass a smooth curve through the waypoints, such as a Catmull-Rom curve, which arrives at each waypoint in the direction its neighbours suggest:

```cpp
const std::vector<P> path = sample_path(waypoints, 20, 0.5);
```
