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

The run that went wrong is the one that cannot be seen, because the robot left the frame.

## Cause

The view was sized from the walls. The picture of the yard was 16.0 m by 9.0 m because it held the poses too.

## Fix

Grow the bounds over everything drawn, including the path, and add a margin.

```cpp
Bounds b = bounds_of(scenario, log);
```
