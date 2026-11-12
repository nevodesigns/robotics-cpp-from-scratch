id: E-SAFE-0026
title: A tilt alarm that goes off in every turn
match: on level ground in a turn the accelerometer alone reports a lean that is not there, and the filter's gate decides how far it is fooled
match: a gate tight enough to ignore a turn lets the gyro's bias build up, and a wider gate pays for it in the turn
platforms: linux, windows
teaches: 19-09-the-slope-and-the-turn
---

## Symptom

The tilt stop fires whenever the robot turns, or the tilt reading wanders on a level floor, and the monitor gets a high threshold or is turned off.

## Cause

On level ground at 4 m/s^2 sideways the accelerometer alone was 27.3 degrees wrong, the filter with a gate of 15 percent 22.1 and with 3 percent 5.1. With a gyro bias of 0.5 deg/s over 60 s the errors were 1.2, 1.7 and, at a gate of 1 percent, 4.1; the 1 percent gate in the turn gave 0.9.

## Fix

Gate on the accelerometer's magnitude and limit the turn so it is not so wrong.

```cpp
if (std::fabs(mag - kGravity) <= gate_ * kGravity) angle_ += k * (measured - angle_);
```
