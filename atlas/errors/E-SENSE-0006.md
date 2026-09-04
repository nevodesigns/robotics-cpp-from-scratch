id: E-SENSE-0006
title: A calibration applied in the wrong direction
match: a calibration applied backwards is worse than none at all
platforms: linux, windows
teaches: 15-02-precise-and-wrong
---

## Symptom

The sensor is calibrated and the readings are worse than they were before,
though not obviously so. Twenty metres reads as 20.24, which is the kind of
number nobody stops to argue with.

Removing the calibration improves things, which makes no sense to anybody.

## Cause

There are two straight lines in a calibration and they are not the same one.

```
raw  = 0.985 * true + 0.42     what the sensor does
true = 1.0152 * raw  - 0.4264  what you have to do about it
```

The first is what a datasheet or a characterisation report gives you. The second
is the correction. Storing the first where the second belongs applies the
sensor's error a second time instead of undoing it.

| truth | corrected | applied backwards | not corrected at all |
|---|---|---|---|
| 0 m | 0.000 | +0.834 | +0.420 |
| 5 m | 0.000 | +0.685 | +0.345 |
| 10 m | 0.000 | +0.536 | +0.270 |
| 20 m | 0.000 | +0.238 | +0.120 |

Every distance is about twice as wrong as doing nothing, and every reading is
still a plausible number, which is why this survives review.

## Fix

Fit the correction rather than transcribing the sensor, and let the code say
which way round it goes.

```cpp
// raw readings in, true values out
const auto fit = rc::core::Calibration::from_samples(raw, truth);
```

`from_samples(raw, truth)` cannot be got backwards without swapping two named
arguments, which is a visible mistake in a way that a pair of loose doubles
named `scale` and `offset` is not.

If you only have the sensor's own coefficients, invert them once, in one place,
with the arithmetic written out:

```cpp
Calibration correction;
correction.scale = 1.0 / sensor_scale;
correction.offset = -sensor_offset / sensor_scale;
```

Then confirm the direction with one reading. Put the sensor at a known distance
well away from zero and check that the corrected value is closer to the truth
than the raw one. A calibration that makes a reading worse is installed
backwards, and that single check catches it in the workshop rather than on the
robot.
