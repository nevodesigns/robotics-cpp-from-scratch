id: E-FIELD-0039
title: A world typed out again by hand
match: the wall is in a different place in the two simulators
match: the Gazebo world was edited and the scenario file was not
match: the goal is a marker with nothing to collide with
platforms: linux, windows
teaches: 22-13-a-world-for-gazebo
---

## Symptom

The two simulators disagree about a result and the cause is a wall that was moved in one of them.

## Cause

The world was typed, not made. A scenario of two patches, two walls, a post and a goal is 8 models; making them from the file leaves nothing to forget.

## Fix

Make the world from the scenario file every time, and edit the file.

```cpp
write_file("yard.sdf", to_world(scenario, robot));
```
