id: E-FIELD-0019
title: An encoder trusted as the distance travelled
match: the odometry says the robot went further than it did, by the same fraction every time
match: a wheel that slips reads long
match: the lidar noise and the dropouts measured from many scans are the ones set
match: an encoder is a tick short at most, and a wrong wheel or a slipping one reads long
platforms: linux, windows
teaches: 22-06-sensors-that-fail-like-the-field
---

## Symptom

Odometry is long by the same percentage on every run, or only on loose ground.

## Cause

A wheel radius wrong by 2 percent read 0.20 m long over 10 m, and a slip of 5 percent read 0.50 m long.

## Fix

Calibrate the scale against a measured distance, and do not trust the encoder on loose ground without another source.

```cpp
const double metres = encoder.distance() * calibrated_scale;
```
