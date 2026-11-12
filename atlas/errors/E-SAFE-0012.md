id: E-SAFE-0012
title: A latency that left out half of the chain
match: the latency that matters is the whole chain from the event to the brake, and the stop path's timeout is part of it
platforms: linux, windows
teaches: 19-04-how-far-it-goes-before-it-stops
---

## Symptom

The sums say the robot stops in 1 m and it takes 2: the sensor delay was counted and the stop path's own timeout, a control tick and the brake were not.

## Cause

A sensor of 0.1 s, a stop path timeout of 0.3 s, a tick of 0.01 s and a brake of 0.15 s give 0.56 s: 1.12 m before the brakes at 2 m/s and 2.12 m in all. A 3 m sight line then allowed 2.41 m/s; with the timeout at 0.1 s, 2.70 m/s.

## Fix

Add every stage between the event and the brake.

```cpp
const double lat = total_latency(sensor, timeout, period, brake);
```
