id: E-FIELD-0041
title: A slope that tilts the wrong way
match: the hill goes down where the scenario says it goes up
match: a positive pitch lowers the x axis
match: a grade along x lowers the front of the slab and a grade along y raises the left
match: the world has a model for everything in the scenario, with its friction, and the robot at its start
platforms: linux, windows
teaches: 22-13-a-world-for-gazebo
---

## Symptom

The robot rolls back down a slope that the scenario says it should climb, or the reverse.

## Cause

A rotation about y by a positive angle turns the x axis downwards, so a rise along +x is a pitch of minus atan of the grade (-0.0996687 rad for 0.1).

## Fix

Work out what the rotated slab does to a step along x, and test the sign.

```cpp
pitch = -std::atan(p.grade_x);
```
