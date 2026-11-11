id: E-SENSE-0036
title: A rejection window wider than the signal can bear
match: the rejection filter holds a real step back, and the smoother adds to it: time to half way, by the settings
platforms: linux, windows
teaches: 15-16-a-sensor-you-can-trust
---

## Symptom

The sensor is clean, and when the joint really does jump, the reading takes a long time to follow.

## Cause

A real step looks like a run of outliers until enough of the window has moved, so the filter holds it back by about half its window. Time to half way with alpha 0.5: 0.01 s at a window of 3, 0.03 s at 7 and 0.07 s at 15, against 0.00 s with no rejection filter. With alpha 0.2 the smoother's delay is added: 0.04, 0.06 and 0.10 s.

## Fix

Choose the smallest window that catches the outliers the sensor has, and measure the step.

```cpp
config.window = 7;
const double t = measure_step_time(config, 0.01);
```
