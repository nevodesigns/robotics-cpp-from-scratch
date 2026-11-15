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

The first command to a new board makes the robot spin, or drive off at full speed the wrong way. A new board was driven straight from a program.

## Cause

Nobody checked which way the encoders count when the wheels are driven forward.

A motor wired backwards drives the wrong way, and an encoder wired backwards counts the wrong way. Either alone makes a speed loop push the wrong way and run away.

## Fix

Run the bring-up checks in order, cheapest first, with the wheels off the ground, before any program that uses speeds. On the fake board the direction check found a reversed left motor and a dead right encoder, each with its own advice.

```cpp
const std::vector<rc::hal::BringUpResult> results = rc::hal::run_bringup(board, expectations, tape);
```
