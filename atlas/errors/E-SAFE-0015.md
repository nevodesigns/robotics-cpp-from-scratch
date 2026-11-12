id: E-SAFE-0015
title: A speed limit from a range that had gone stale
match: driving at a wall with the limiter on, the robot slows to a stop short of it, and without it, it arrives at full speed
match: a sensor that goes quiet in the middle of the approach stops the robot, and one whose readings go stale does too
platforms: linux, windows
teaches: 19-05-a-speed-the-sensor-can-vouch-for
---

## Symptom

The sensor stops publishing, and the robot goes on at the speed its last good range allowed.

## Cause

With the limiter the robot driving at a wall at a command of 2 m/s came no closer than 0.30 m; without it it hit at 2.00 m/s. A reading of 4.0 m allowed 2.00 m/s at 0.05 s old and 0.00 at 0.25 s; at exactly 0.2 s it was still fresh; a NaN allowed 0.00.

## Fix

Keep the age with every reading and refuse the old ones.

```cpp
if (r.age > c.max_age) return 0.0;
```
