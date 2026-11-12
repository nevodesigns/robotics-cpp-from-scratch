id: E-SAFE-0010
title: A timeout that was chosen without a speed
match: the formula matches a robot that is actually stopped step by step
match: the largest speed for a sight line is the inverse of the stopping distance, with the margin kept
platforms: linux, windows
teaches: 19-04-how-far-it-goes-before-it-stops
---

## Symptom

The stop path has a sensible timeout and the robot still hits what it was driving at, because the timeout was never turned into a distance.

## Cause

At 2 m/s and 2 m/s^2 with a latency of 0.3 s the stopping distance was 1.600 m, 0.6 m of it before the brakes. The speed a sight line allows grew with its square root: 3.654 m/s at 5 m and 5.512 at 10 m. At a sight line of 0.2 m, the size of the margin, it was 0.

## Fix

Compute the speed from the sight line and keep to it.

```cpp
const double vmax = max_speed(sight, decel, latency, margin);
```
