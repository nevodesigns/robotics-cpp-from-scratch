id: E-SENSE-0037
title: A filter added without measuring what it costs
match: what is left of the noise is close to what the smoother predicts, the square root of alpha over 2 minus alpha
match: the output runs behind a ramp by the smoother's delay, (1 minus alpha) over alpha samples
match: a quieter output lags more, and the table of the four costs is what to choose from
platforms: linux, windows
teaches: 15-16-a-sensor-you-can-trust
---

## Symptom

The sensor trace is beautifully clean, and the arm it drives is sluggish and rings when the load changes.

## Cause

Every bit of quiet is bought with delay. Measured at 100 Hz:

| alpha | noise left | ramp lag (s) |
|---|---|---|
| 1.00 | 0.939 | 0.0000 |
| 0.50 | 0.557 | 0.0100 |
| 0.20 | 0.339 | 0.0400 |
| 0.05 | 0.173 | 0.1900 |

The delay goes into the control loop's budget (lesson 14-12) whether it was counted or not.

## Fix

Measure the costs before the filter goes on the robot.

```cpp
const double lag = measure_ramp_lag(config, 0.01);
const double noise = measure_noise_ratio(config);
```
