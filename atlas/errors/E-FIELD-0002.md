id: E-FIELD-0002
title: A stop that is taken to be instant
match: a stop command is followed by motion for the time constant of the base
match: the base coasted after stop
match: a command is limited, answered with a lag of the time constant, and held
match: a base that stops hearing commands stops itself, and a stop coasts for the time constant
platforms: linux, windows
teaches: 22-01-a-robot-as-an-interface
---

## Symptom

The robot stops a hand's width too late every time, and the planner's margin is wrong by exactly that much. The planner's stopping margin was set from the command, not from the base.

## Cause

The stop was taken as instant. A base answers a command with a lag: a stop from 1 m/s coasted 0.200 m, the speed times a time constant of 0.2 s.

Motors and their drivers answer a change of command over a time constant: the speed falls exponentially, not at once. In that time the base keeps moving, and the distance covered is about the speed times the time constant.

## Fix

Plan with the base's stopping distance, at least the speed times its time constant, and measure the time constant on the real base.

```cpp
const double coast = speed * limits.tau;
```
