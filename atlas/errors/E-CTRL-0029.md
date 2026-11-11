id: E-CTRL-0029
title: A derivative gain raised with no filter
match: in a loop the motor chatters at the sensor's noise unless the derivative is filtered
platforms: linux, windows
teaches: 14-11-the-noise-the-derivative-loves
---

## Symptom

More kd was meant to calm an overshoot and the motor now buzzes, hot and loud, while the overshoot barely changed.

## Cause

The chatter is the derivative of the noise times kd, so it scales with the gain. In a loop with kp 40 and kd 4 on a sensor good to 0.001 rad, the change in force from one step to the next:

| derivative | force step, rms | error at the end |
|---|---|---|
| no filter | 9.373 N | -0.0001 rad |
| tf 0.02 s | 0.317 N | 0.0000 rad |

## Fix

Filter before raising kd, and check the force the motor is asked for, not only the position.

```cpp
const double force = kp * (target - z) - kd * derivative.update(z, dt);
```
