id: E-SENSE-0025
title: One time constant chosen for every case
match: one time constant cannot make both errors small, and the total is least between the extremes
platforms: linux, windows
teaches: 15-11-two-sensors-one-angle
---

## Symptom

A filter that was tuned on one robot is jittery on another, or slow and off on a third.

## Cause

The right tau depends on the accelerometer's noise and the gyro's bias, and both change from one device to the next. Error while swinging, measured:

| tau (s) | rms (rad) |
|---|---|
| 0.05 | 0.0144 |
| 0.20 | 0.0082 |
| 1.00 | 0.0205 |
| 20.00 | 0.3202 |

## Fix

Measure both errors on the device, as the test does, and choose tau from the table.

```cpp
ComplementaryFilter filter(0.2);
```
