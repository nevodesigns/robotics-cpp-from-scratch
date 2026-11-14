id: E-FIELD-0009
title: Braking assumed equal to acceleration
match: the stopping distance is planned with the acceleration limit
match: braking is stronger than accelerating
match: a limit read in the wrong unit makes the stopping distance thirteen times as long
platforms: linux, windows
teaches: 22-03-units-and-limits-that-cannot-be-mixed
---

## Symptom

Stops are planned far longer than they need to be, or the reverse when the braking limit is the weaker one.

## Cause

One number was used for both. A ramp to 1 m/s took 2.000 s and the stop from it 0.667 s.

## Fix

Keep the two limits apart, and take each from a measurement of the vehicle.

```cpp
const double allowed = (fabs(v) > fabs(prev_v) ? lim.max_accel : lim.max_decel) * dt;
```
