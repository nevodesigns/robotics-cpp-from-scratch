id: E-SAFE-0011
title: A braking figure from a dry flat floor
match: the ground and gravity decide the braking, and on a wet slope there is none to be had
match: latency costs speed, and a second of it costs more than two fifths
platforms: linux, windows
teaches: 19-04-how-far-it-goes-before-it-stops
---

## Symptom

The robot stops in the distance on a test day and slides through the fence on a wet slope.

## Cause

Brakes of 3 m/s^2 gave 3.00 dry and flat, 1.30 dry on 10 degrees downhill, 1.19 wet on 10 degrees downhill and 0.00 wet on 17 degrees downhill, where the stopping distance was infinite. A second of latency took 43 percent off the speed a 3 m sight line allowed.

## Fix

Use the worst floor and slope the robot will meet.

```cpp
const double a = effective_decel(brake, worst_friction, worst_slope);
```
