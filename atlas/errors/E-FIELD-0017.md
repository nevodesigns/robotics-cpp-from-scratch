id: E-FIELD-0017
title: A gyro bias taken to be zero
match: the heading drifts steadily while the robot is at rest
match: a gyro bias of 0.01 rad/s turns the heading by 34 degrees in a minute
match: a gyro with a bias of 0\.01 rad/s turns the heading by about 34 degrees a minute while the robot sits still
platforms: linux, windows
teaches: 22-06-sensors-that-fail-like-the-field
---

## Symptom

The heading from the gyro walks away while the robot stands still, and gets worse with time. The robot was started straight after power-up.

## Cause

The gyro's bias was taken to be zero. Integrating the rate integrates the bias: 34.1 degrees in a minute at 0.01 rad/s.

A gyro's output is the turn rate plus a bias. Integrating to get a heading integrates the bias, which becomes an angle growing steadily with time.

## Fix

Estimate the bias when the robot is known to be still, and correct the heading from another source.

```cpp
const double w = gyro.gyro_z - bias_estimate;
```
