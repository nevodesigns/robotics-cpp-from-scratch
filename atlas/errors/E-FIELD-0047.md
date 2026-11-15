id: E-FIELD-0047
title: A gap read as one simulator being wrong
match: the two simulators disagree so one of them must have a bug
match: calibrating one simulator to the other and calling it validated
match: two built in models with different lags differ, the lag is read from the slower one, and set in the faster one they agree
platforms: linux, windows
teaches: 22-15-two-simulators-and-the-gap
---

## Symptom

After the built in model is calibrated to Gazebo the two agree, and the model is said to be validated.

## Cause

A gap between two simulations was read as one of them being wrong, and closing it as validation. Agreement between two simulations is a statement about the two: the worst gap after calibration was 0.035 m, and whether either matches the robot is a separate measurement.

## Fix

Say what was calibrated and to what, and keep a hardware check for any claim about the robot.

```cpp
// calibrated to Gazebo's step response, not validated against the robot
```
