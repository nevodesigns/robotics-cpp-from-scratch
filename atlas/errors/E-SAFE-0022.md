id: E-SAFE-0022
title: A power cut-off that has no warning before it
match: power falls smoothly from full at the warning voltage to nothing at the stop voltage, and the temperature does the same
platforms: linux, windows
teaches: 19-08-the-battery-and-the-heat
---

## Symptom

The robot stops dead, mid task, with a battery that was reported fine a minute before, in a place that was not safe to stop in.

## Cause

The share of power fell 1.00, 1.00, 0.75, 0.50, 0.25, 0.05 from 24.0 V down to 20.1 V at rest and was 0 at 20.0 V; at 70 degrees it was 0.50 and at 80 it stopped.

## Fix

Reduce the power over a band before the stop.

```cpp
const double vf = clamp01((rest - cfg.volt_stop) / (cfg.volt_warn - cfg.volt_stop));
```
