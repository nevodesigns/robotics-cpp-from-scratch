id: E-FIELD-0008
title: A cap on speed that ignores the turn
match: a vehicle that is safe on a straight line slides in a tight turn
match: speed times turn rate exceeds the lateral acceleration limit
match: a fast turn at speed is cut by speed times turn rate, and a straight line is not
match: a command beyond the limits is flagged as clamped
platforms: linux, windows
teaches: 22-03-units-and-limits-that-cannot-be-mixed
---

## Symptom

The vehicle is fine on the straight and tips or slides when a planner asks for a sharp turn at speed.

## Cause

Only the speed was limited. 1 m/s at 2 rad/s is a lateral acceleration of 2 m/s2; the limit of 1.5 cut the speed to 0.750 m/s.

## Fix

Cut the speed so that speed times turn rate stays within the lateral limit.

```cpp
v = std::copysign(lim.max_lateral / std::fabs(w), v);
```
