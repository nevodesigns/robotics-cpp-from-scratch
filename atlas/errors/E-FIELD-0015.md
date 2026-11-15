id: E-FIELD-0015
title: A planner that sends a robot where it cannot climb
match: the robot stalls halfway up a slope that the motors could drive
match: the slope is steeper than the surface allows
match: there is a steepest slope a surface can be climbed or held on
platforms: linux, windows
teaches: 22-05-terrain-and-friction
---

## Symptom

The robot stops or slides back on a slope with its motors at full effort.

## Cause

The planner sent it where the surface could not hold it. The motors were not the limit; the grip was: gravel can be climbed up to 22.3 degrees and ice up to 5.1.

## Fix

Ask whether the cell can be climbed on that heading before planning through it.

```cpp
if (!rc::field::traction(world.at(x, y), heading).can_climb) cost = infinity;
```
