id: E-FIELD-0010
title: A controller tuned without the time constant of the motor
match: a speed loop overshoots because the motor lags by its electromechanical time constant
match: the simulated motor reaches 63 percent in the time the formula gives
platforms: linux, windows
teaches: 22-04-motors-gearboxes-and-backlash
---

## Symptom

A gain that worked on a model with an instant motor rings or oscillates on the robot.

## Cause

The motor answers a voltage with a lag of 0.246 s for this load, and the model had none.

## Fix

Compute the time constant from J, R and the constants, check it against a step, and tune with it.

```cpp
const double tau = motor.time_constant();
```
