id: E-CTRL-0027
title: A raw derivative on a noisy sensor
match: a filter on the derivative cuts the noise by a factor that grows with its time constant
match: the first reading gives zero, and a steady slope comes through once the filter has settled
platforms: linux, windows
teaches: 14-11-the-noise-the-derivative-loves
---

## Symptom

A joint that is standing still has a motor that buzzes. The derivative term reports a speed that is never zero.

## Cause

A derivative divides the difference of two readings by dt, and the sensor's error is in both. Measured, rms of the derivative of a sensor good to 0.001 rad at 1 kHz:

| tf (s) | rms (rad/s) |
|---|---|
| 0.000 | 1.4067 |
| 0.005 | 0.1737 |
| 0.020 | 0.0480 |
| 0.050 | 0.0196 |

A slope of 3 rad/s read 1.869 after one time constant and 3.000 after ten, so the filter passes real motion once it has settled.

## Fix

Low pass the derivative, with the share of each new value set from a time constant.

```cpp
DerivativeFilter derivative(0.02);
const double rate = derivative.update(position, dt);   // alpha = dt / (tf + dt)
```
