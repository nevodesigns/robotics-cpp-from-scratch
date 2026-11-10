id: E-MATH-0027
title: An angle taken from a quaternion that is not a number
match: the angle of a rotation, and the angle between two directions
match: expected this to be true: !std::isnan
platforms: linux, windows
teaches: 06-13-random-in-every-direction
---

## Symptom

A measure of how far two orientations differ returns not a number now and then, for rotations that are the same to rounding, and the average of it is not a number for ever after.

## Cause

The angle of a rotation is 2 times the arc cosine of the scalar part of its quaternion. Rounding can leave that a hair over 1, and the arc cosine of a number over 1 is not a number. Also, a quaternion and its negative are the same rotation, so the sign of the scalar part means nothing and the angle has to use its size.

Measured: a scalar part of 1 + 1e-15 gave a number, not `nan`; a quaternion with every sign flipped gave the same angle; a quarter turn about z gave pi/2 and a half turn pi.

## Fix

```cpp
double w = std::fabs(q.w);
if (w > 1.0) w = 1.0;
return 2.0 * std::acos(w);
```
