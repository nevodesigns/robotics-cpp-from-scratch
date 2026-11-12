id: E-FIELD-0054
title: A robot with its weight over the axle
match: the simulated robot turns short after it has driven
match: the caster only touches the floor when the body rocks
platforms: linux, windows
teaches: 22-12-describing-a-robot-once
---

## Symptom

A turn from rest is right and the same turn after a drive is short by a third, and the simulator seems to be wrong about friction.

## Cause

The centre of mass was over the axle, so the robot rested on two wheels and a caster that touched only when the body rocked. Friction was not the cause: changing it changed nothing. Moving the weight 14.6 mm behind the axle fixed it.

## Fix

Put the weight between the axle and the caster, and check with a turn after a drive, not only from rest.

```cpp
com_back 0.0144 source typical
```
