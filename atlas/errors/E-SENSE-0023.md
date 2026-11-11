id: E-SENSE-0023
title: A gyro integrated alone
match: a gyro alone drifts by its bias times the time, and an accelerometer alone is as noisy as it is
match: a filter that believes both is better than either alone
platforms: linux, windows
teaches: 15-11-two-sensors-one-angle
---

## Symptom

The angle from a gyro is smooth, and a few minutes later it is wrong by more than the joint's range, with the joint standing still.

## Cause

Every gyro reading is off by a small bias and the integral adds them all up. A still joint, a gyro reading 0.02 rad/s high, errors from the tenth to the sixtieth second:

| source | rms error (rad) |
|---|---|
| gyro alone | 0.764 (a ramp predicts 0.757) |
| accelerometer alone | 0.050 |
| complementary filter, tau 1 s | 0.020 |

## Fix

Give the integral something that does not drift to be pulled toward.

```cpp
ComplementaryFilter filter(1.0);
const double angle = filter.update(gyro_rate, accel_angle, dt);
```
