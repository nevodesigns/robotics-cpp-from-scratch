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

Odometry is long by the same percentage on every run, or only on loose ground. The wheel radius was taken from the tyre's marking.

## Cause

The encoder was trusted as the distance travelled. A wheel radius wrong by 2 percent read 0.20 m long over 10 m, and a slip of 5 percent read 0.50 m long.

Encoder distance is ticks times the circumference over ticks per turn. A radius wrong by a few percent makes every metre wrong by the same share, and slip adds turns that went nowhere.

## Fix

Calibrate the scale against a measured distance, and do not trust the encoder on loose ground without another source.

```cpp
const double metres = encoder.distance() * calibrated_scale;
```
