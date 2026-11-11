id: E-CTRL-0030
title: Gains tuned for a continuous loop on a slow controller
match: a slower controller overshoots more, and the overshoot grows faster the slower it gets
match: past a limit the same gains no longer hold the joint
platforms: linux, windows
teaches: 14-12-the-rate-the-loop-runs-at
---

## Symptom

A loop that behaved in simulation overshoots and rings on the robot, and past a certain slowness it will not hold still at all.

## Cause

The controller acts on a reading that is on average half a period old and holds its answer for a whole period. Measured at kp 100 and kd 10:

| period (s) | overshoot |
|---|---|
| 0.001 | 0.156 |
| 0.050 | 0.228 |
| 0.100 | 0.398 |
| 0.150 | 0.791 |

At 0.3 s the loop was not stable. The largest stable period was 0.1821 s.

## Fix

Find the largest stable period for the gains and run well inside it.

```cpp
const double limit = largest_stable_period(kp, kd);
```
