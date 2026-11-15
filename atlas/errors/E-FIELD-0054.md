id: E-FIELD-0054
title: A robot with its weight over the axle
match: the simulated robot turns short after it has driven
match: the caster only touches the floor when the body rocks
match: the weight is put behind the axle, towards the caster, so that the robot rests on three points and does not rock
platforms: linux, windows
teaches: 22-12-describing-a-robot-once
---

## Symptom

A turn from rest is right, the same turn after a drive is short by a quarter, and the simulator seems to be wrong about friction. It is the simulator's model of a robot with two wheels and a caster.

## Cause

The robot's weight was over its axle, so it rested on two wheels and a caster that touched the floor only when the body rocked, which driving and stopping do. Friction was not the cause: changing it changed nothing.

Three points of contact make a stable base only if the weight is inside the triangle they make. With the weight on the axle line, the robot balances on its two wheels and the caster carries nothing until the body tips.

## Fix

Put the weight between the axle and the caster, and check with a turn after a drive, not only from rest. The robot file now puts the whole robot's centre of mass 40 percent of the way to the caster, 14.4 mm behind the axle (the chassis' own centre is 14.6 mm behind it), and the turns after a drive came right.

```cpp
com_back 0.0144 source typical
```

The line above is the optional com_back line of the robot file. Lesson 23-09 writes com_back 0.001 to bring the fault back and measure it.
