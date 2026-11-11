id: E-SENSE-0024
title: A complementary time constant chosen too long
match: a gyro bias leaves a steady error of the bias times tau, so a longer tau trusts the gyro more
platforms: linux, windows
teaches: 15-11-two-sensors-one-angle
---

## Symptom

The fused angle is smooth and steadily off by a fixed amount, which grows when the filter is made smoother.

## Cause

The pull toward the accelerometer is the only thing holding the bias back, and a long tau weakens it. The steady error is b times tau. Measured with a bias of 0.02 rad/s:

| tau (s) | error (rad) |
|---|---|
| 0.25 | 0.0050 |
| 1.00 | 0.0200 |
| 4.00 | 0.0800 |

## Fix

Keep tau as short as the accelerometer's noise allows, and calibrate the bias out (lesson 15-02) before filtering.

```cpp
const double tau = tau_for_crossover(0.5);
```
