id: E-SAFE-0025
title: A payload that raised the centre of gravity
match: a vehicle tips when the sideways pull outweighs its weight, and a payload up high lowers both the angle and the pull it can take
match: standing still on a slope, the accelerometer gives the roll and the pitch
platforms: linux, windows
teaches: 19-09-the-slope-and-the-turn
---

## Symptom

The robot is safe on the bench and tips in a turn in the field, with a payload on it that was not there at the test.

## Cause

A track of 0.5 m tipped at 8.18 m/s^2 and 39.8 degrees with the centre of gravity at 0.3 m, at 6.13 and 32.0 at 0.4 m and at 4.91 and 26.6 at 0.5 m. A radius 1 m turn at half the tipping acceleration allowed 2.02 m/s, and 1.57 m/s (77 percent) with the payload.

## Fix

Put the centre of gravity with the payload on into the limit.

```cpp
const double v = max_turn_speed(curvature, track, cg_height_with_payload, 0.5);
```
