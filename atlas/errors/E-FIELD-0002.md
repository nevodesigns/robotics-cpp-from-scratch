id: E-FIELD-0002
title: A stop that is taken to be instant
match: a stop command is followed by motion for the time constant of the base
match: the base coasted after stop
platforms: linux, windows
teaches: 22-01-a-robot-as-an-interface
---

## Symptom

The robot stops a hand's width too late every time, and the margin in the planner is wrong by exactly that much.

## Cause

The base answers a command with a lag. A stop at 1 m/s coasted 0.200 m, the speed times a time constant of 0.2 s.

## Fix

Plan with the stopping distance of the base, speed times time constant at least, and measure it on the real base.

```cpp
const double coast = speed * limits.tau;
```
