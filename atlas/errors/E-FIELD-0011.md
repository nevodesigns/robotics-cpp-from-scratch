id: E-FIELD-0011
title: A current limit forgotten in the torque budget
match: the robot cannot climb a slope that the stall torque says it can
match: the torque is limited by the driver current, not by the motor
match: the data sheet numbers come back out: the no load speed, and half the speed at half the stall torque
match: the current limit caps the torque, and a stronger limit does not help past stall
platforms: linux, windows
teaches: 22-04-motors-gearboxes-and-backlash
---

## Symptom

The robot stalls on a slope the data sheet's torque says is easy.

## Cause

The driver limited the current, and the torque budget did not know. With a limit of 1 A the stall torque was 0.312 N m, not the sheet's 0.700.

## Fix

Use the smaller of the stall torque and what the current limit allows.

```cpp
const double torque = motor.max_torque_out(volts);
```
