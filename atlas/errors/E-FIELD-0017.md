id: E-FIELD-0017
title: A gyro bias taken to be zero
match: the heading drifts steadily while the robot is at rest
match: a gyro bias of 0.01 rad/s turns the heading by 34 degrees in a minute
platforms: linux, windows
teaches: 22-06-sensors-that-fail-like-the-field
---

## Symptom

The heading from the gyro walks away while the robot is stationary, and gets worse with time.

## Cause

Integrating the gyro integrates its bias: 34.1 degrees in a minute at 0.01 rad/s.

## Fix

Estimate the bias when the robot is known to be still, and correct the heading from another source.

```cpp
const double w = gyro.gyro_z - bias_estimate;
```
