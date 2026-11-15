id: E-FIELD-0027
title: A view sized to the walls
match: the robot leaves the picture and the failure cannot be seen
match: the bounds do not include the path
match: the bounds hold everything that is drawn
match: the picture is well formed, has one element for each thing in the scenario, and one point for each pose
platforms: linux, windows
teaches: 22-09-watching-a-run
---

## Symptom

The run that went wrong is the one that cannot be seen, because the robot left the frame. The robot left the walls.

## Cause

The view was sized to the walls. A robot that went outside them was drawn off the picture.

A view sized to the scenario's walls shows what was planned. The interesting runs are the ones where the robot went somewhere unplanned, which is outside that view.

## Fix

Grow the bounds over everything drawn, the path included, and add a margin. The picture of the yard became 16.0 m by 9.0 m because it held the poses too.

```cpp
const rc::field::Bounds b = rc::field::bounds_of(scenario, log);
```
