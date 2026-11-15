id: E-FIELD-0005
title: A tilted sensor treated as flat
match: the ground is read as a wall or a gap because the sensor mount has a tilt
match: a lidar tipped down 5 degrees sees the ground at a finite range
match: a lidar tipped down sees the ground at a range that is not its height divided by anything simple
platforms: linux, windows
teaches: 22-02-frames-for-a-field-robot
---

## Symptom

The robot sees a wall ahead on flat ground, or misses a kerb, depending on how the sensor is mounted. The sensor was mounted with a downward tilt.

## Cause

The scan was used as if the sensor looked along the ground. A sensor 0.25 m up and tipped 5 degrees down meets flat ground 2.858 m ahead, and a tilted scan read as flat puts that ground in the way.

A tilted sensor's beams meet the ground ahead. In the sensor's frame those points are straight in front; only through the mount's pitch are they on the floor. Without it, the floor reads as a wall at the distance where the beams meet it.

## Fix

Put the mount in the frame tree, with its pitch, and take the points through it before they reach the planner.

```cpp
tree.add("lidar", "base", rc::field::from_rpy(0.0, 0.0, 0.25, 0.0, tilt, 0.0));
```
