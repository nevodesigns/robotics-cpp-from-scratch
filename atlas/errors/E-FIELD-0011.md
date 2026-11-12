id: E-FIELD-0011
title: A current limit forgotten in the torque budget
match: the robot cannot climb a slope that the stall torque says it can
match: the torque is limited by the driver current, not by the motor
platforms: linux, windows
teaches: 22-04-motors-gearboxes-and-backlash
---

## Symptom

The robot stalls on a slope that the data sheet torque says is easy.

## Cause

The driver limited the current. With a limit of 1 A the stall torque was 0.312 N m, not 0.700 N m.

## Fix

Use the smaller of the stall torque and what the current limit gives.

```cpp
const double torque = motor.max_torque_out(volts);
```
