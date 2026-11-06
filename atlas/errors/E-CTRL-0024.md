id: E-CTRL-0024
title: A speed made by differencing a noisy position
match: a plain difference of two readings is a hundred times noisier than the observer
match: expected this to be true: e.difference
platforms: linux, windows
teaches: 14-10-the-speed-you-cannot-measure
---

## Symptom

A joint holds position well, and the moment a derivative or speed term is added the motor buzzes and runs warm. The position
trace looks clean. The speed trace is a thick fuzz, and turning the derivative gain down to quiet it also removes the damping
it was added for.

## Cause

The speed is (new reading minus old reading) / dt. The noise on each reading is the same size whether the joint moves or not,
and dividing by a millisecond multiplies it by a thousand. Two readings each wrong by up to a thousandth of a radian give a speed
wrong by a radian a second.

Measured, with a sensor good to 0.001 rad read at 1 kHz on a swing whose peak speed is 3.1 rad/s: the difference had **1.422 rad/s**
rms error and an observer at a bandwidth of 50 had **0.006**, over 200 times less.

## Fix

Estimate the speed instead of computing it. Predict the next position from the speed already believed and the acceleration
commanded, and correct both by the position error.

```cpp
observer.update(measured_position, commanded_acceleration, dt);
const double speed = observer.velocity();
```

A filter on the difference trades the noise for lag and cannot know the acceleration, which the observer uses.
