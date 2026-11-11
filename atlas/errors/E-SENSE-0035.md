id: E-SENSE-0035
title: Smoothing placed before outlier rejection
match: the stages run in order: calibrated first, so a steady raw reading comes out as scale times it plus offset
match: rejecting wild readings first keeps a single spike from leaking through the smoother
platforms: linux, windows
teaches: 15-16-a-sensor-you-can-trust
---

## Symptom

One bad reading produces a small bump that decays over many samples, and the controller reacts to it.

## Cause

A smoother mixes a share of each reading, alpha, into every later output. Applied before rejection it takes a spike of 50 and turns it into a smear the rejection filter can no longer recognise. Measured with the smoother at alpha 0.2: the output peaked at 0.000 of the spike when rejection came first and 0.200 when there was none. Calibration goes first of all, so the stages work in real units: a raw 3.0 became 7.0000 through a scale of 2 and an offset of 1.

## Fix

Calibrate, reject, then smooth.

```cpp
const double calibrated = config_.calibration.apply(raw);
const double clean = hampel_.update(calibrated);
return smoother_.update(clean);
```
