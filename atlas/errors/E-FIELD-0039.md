id: E-FIELD-0039
title: A world typed out again by hand
match: the wall is in a different place in the two simulators
match: the Gazebo world was edited and the scenario file was not
match: the goal is a marker with nothing to collide with
match: a wall that does not start at the origin is as long as it is and sits at its own middle
platforms: linux, windows
teaches: 22-13-a-world-for-gazebo
---

## Symptom

The two simulators disagree, and the cause is a wall that was moved in one of them.

## Cause

The world was typed out again by hand. A scenario of two patches, two walls, a post and a goal is 8 models, and each is a place to forget a change.

## Fix

Make the world from the scenario file every time, and edit only the file.

```cpp
write_file("yard.sdf", rc::field::to_world(scenario, robot));
```
