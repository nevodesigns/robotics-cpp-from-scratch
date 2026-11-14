id: E-FIELD-0048
title: A first drive with no direction check
match: the robot spins on the spot at full power the first time it is given a speed
match: one motor runs the wrong way
match: a healthy board passes the ten checks, in the order that costs least first
match: the numbers measured are the ones the fault set, and the advice says what to do about it
platforms: linux, windows
teaches: 22-16-a-bring-up-checklist-for-a-board
---

## Symptom

The first command on a new board makes it spin, or drive off at full speed the wrong way.

## Cause

Nobody checked which way the encoders count when the wheels are driven forward. The direction check found a reversed left motor and a dead right encoder in the fake, each with its own advice.

## Fix

Run the checks in order, cheapest first, with the wheels off the ground, before any program that uses speeds.

```cpp
const auto results = run_bringup(board, expectations, tape);
```
