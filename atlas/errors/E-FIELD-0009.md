id: E-FIELD-0009
title: Braking assumed equal to acceleration
match: the stopping distance is planned with the acceleration limit
match: braking is stronger than accelerating
match: a limit read in the wrong unit makes the stopping distance thirteen times as long
platforms: linux, windows
teaches: 22-03-units-and-limits-that-cannot-be-mixed
---

## Symptom

Stops are planned far longer than they need be, or shorter when braking is the weaker limit. The limits were copied from a model that used one number for both.

## Cause

One number was used for speeding up and for slowing down. Here a ramp to 1 m/s took 2.000 s and the stop from it 0.667 s: braking is three times the acceleration.

Motors limit acceleration; brakes and the tyres' grip limit deceleration, and the two are rarely equal. One number for both is wrong for at least one of them.

## Fix

Keep the two limits apart, and take each from a measurement of the vehicle.

```cpp
const double allowed = (std::fabs(v) > std::fabs(prev_v) ? lim.max_accel : lim.max_decel) * dt;
```
