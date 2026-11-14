id: E-FIELD-0015
title: A planner that sends a robot where it cannot climb
match: the robot stalls halfway up a slope that the motors could drive
match: the slope is steeper than the surface allows
match: there is a steepest slope a surface can be climbed or held on
platforms: linux, windows
teaches: 22-05-terrain-and-friction
---

## Symptom

The robot stops or slides back on a slope, with the motors at full effort.

## Cause

The motors were not the limit; the surface was. Gravel can be climbed up to 22.3 degrees and ice to 5.1.

## Fix

Ask traction() whether the cell can be climbed before planning through it.

```cpp
if (!traction(world.at(x, y), heading).can_climb) cost = infinity;
```
