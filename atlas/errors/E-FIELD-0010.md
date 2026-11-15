id: E-FIELD-0010
title: A controller tuned without the time constant of the motor
match: a speed loop overshoots because the motor lags by its electromechanical time constant
match: the simulated motor reaches 63 percent in the time the formula gives
match: the time constant from the constants is the time a step takes to reach 63 percent
platforms: linux, windows
teaches: 22-04-motors-gearboxes-and-backlash
---

## Symptom

A gain that worked on a model with an instant motor rings or oscillates on the robot. The controller was tuned in a model with an ideal motor.

## Cause

The model had no motor lag. A motor answers a voltage with a time constant set by its inertia, resistance and constants: 0.246 s for this load.

A motor's speed answers a voltage through its inertia and its electrical and mechanical constants. A controller tuned on a model with no lag has more gain than the real loop can take, and the lag turns the extra gain into oscillation.

## Fix

Work the time constant out from J, R and the constants, check it against a step on the motor, and tune with it in the model.

```cpp
const double tau = motor.time_constant();   // J R / (ke^2 + b R)
```
