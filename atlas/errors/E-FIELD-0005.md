id: E-FIELD-0005
title: A tilted sensor treated as flat
match: the ground is read as a wall or a gap because the sensor mount has a tilt
match: a lidar tipped down 5 degrees sees the ground at a finite range
match: a lidar tipped down sees the ground at a range that is not its height divided by anything simple
platforms: linux, windows
teaches: 22-02-frames-for-a-field-robot
---

## Symptom

The robot sees a wall in front of it on flat ground, or misses a kerb, depending on the mounting of the sensor.

## Cause

The scan was used as if the sensor looked along the ground. At 0.25 m and 5 degrees down the ground is at 2.858 m.

## Fix

Put the mount in the tree, with its pitch, and take the points through it before they reach the planner.

```cpp
tree.add("lidar", "base", from_rpy(0.0, 0.0, 0.25, 0, tilt, 0));
```
