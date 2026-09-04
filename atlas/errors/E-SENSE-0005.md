id: E-SENSE-0005
title: A calibration taken at one point is exact there and wrong everywhere else
match: one reference is exact where you tested it and wrong at both ends
platforms: linux, windows
teaches: 15-02-precise-and-wrong
---

## Symptom

The sensor was zeroed against a known distance and checked, and it was perfect.
In use it reads a little long close up and a little short far away, or the other
way round, and nobody can find the mistake because the calibration was verified.

Re-zeroing moves the error rather than removing it. The place it was zeroed is
always right.

## Cause

One reference can only ever fix an offset, and the sensor also has a scale
error. A single point has infinitely many lines through it, and zeroing picks
the one with a slope of exactly 1, which is a guess.

The same rangefinder, `0.985 x + 0.42`, zeroed at 10 m:

| truth | raw | corrected | error |
|---|---|---|---|
| 0 m | 0.420 | 0.150 | **+0.150** |
| 5 m | 5.345 | 5.075 | +0.075 |
| 10 m | 10.270 | 10.000 | 0.000 |
| 15 m | 15.195 | 14.925 | -0.075 |
| 20 m | 20.120 | 19.850 | **-0.150** |

Exact at the one place it was checked, and wrong at both ends by the same amount
in opposite directions.

That signature is the whole diagnosis, and it is worth learning to read:

- **Wrong by the same amount everywhere.** An offset. One reference fixes it.
- **Wrong in opposite directions at the two ends, right in the middle.** A scale
  error being corrected by an offset. One reference cannot fix it.
- **Wrong in the same direction at both ends, right in the middle.** Not a line
  at all. The sensor bends, and no straight correction will do.

## Fix

Take at least two references, and take them apart.

```cpp
const auto fit = rc::core::Calibration::from_two_points(near_raw, 1.0, far_raw, 9.0);
```

Two solves for both the scale and the offset, which is the whole of a straight
line and therefore the whole of the correction, and it will then be right at
distances you never tested.

Place them across the range the robot actually works over. A pair of references
close together is a fit that is excellent between them and drifts once you leave:
a line fitted over 0 to 2 m on a sensor that bends slightly is off by 33 cm at
20 m and 1.39 m at 40 m, while the same sensor fitted over 0 to 20 m is off by
9 cm at worst.

Then check the fit rather than trusting it. `rms_residual` says how far the line
misses the references it was made from; compare it against the sensor's own
noise, because a bend smaller than the noise will not show and is still there.
