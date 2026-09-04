id: E-SENSE-0010
title: Latency that jitters, mistaken for sensor noise
match: latency that jitters looks like sensor noise and is not
match: carrying forward at a rate you only half know is still most of the fix
platforms: linux, windows
teaches: 15-03-late-is-wrong
---

## Symptom

A sensor that is specified as accurate to a few millimetres reports positions
that scatter by a couple of centimetres. Filtering it helps a little, or does
not help, or makes things slightly worse. Replacing the sensor with a better one
changes nothing.

The scatter is worse when the robot moves faster, and absent when it is parked,
which nobody notices because a noisy sensor is expected to look noisy.

## Cause

The latency is not constant. A reading is 4 ms old sometimes and 36 ms old at
other times, depending on where the sampling instant fell relative to the poll,
what else was on the bus, and what the scheduler was doing.

Believed to be about now, each reading is wrong by its own age times the speed.
Varying age, varying error, and the result looks exactly like noise.

Measured at 1 m/s with the latency jittering between 4 and 36 ms:

| | rms error | worst |
|---|---|---|
| believed to be about now | 0.0222 m | 0.0360 m |
| and then smoothed over 16 samples | 0.0350 m | |
| carried forward from when it was taken | 0.0000 m | 0.0000 m |

Two things in that table are worth keeping.

**Smoothing makes it worse.** The filter adds its own 15 ms of lag to the 20 the
bus already cost, so the average reading is older than the raw one was. This is
not zero mean noise on the value; it is the value being about the wrong moment,
and the average of a set of wrong moments is an older wrong moment.

**It is not noise at all.** With the timestamp the error is gone entirely,
because there was never anything random in it.

## Fix

Stamp the reading at the source and carry it forward, rather than filtering it.

```cpp
const auto here = rc::sensor::carried_forward(reading, speed_estimate, clock.now());
```

The rate is itself an estimate and it does not have to be good. What is left
after the correction is the original error times how wrong the rate was:

| rate estimate | error left |
|---|---|
| exact | 0% |
| 10% out | 10% |
| 25% out | 25% |
| 50% out | 50% |

Half a rate estimate removes half the problem, and there is no threshold below
which it stops being worth doing.

The diagnostic that separates this from real sensor noise takes one experiment:
park the robot and measure the scatter, then drive it at 1 m/s and measure again.
Noise is the same in both. This grows with speed, and disappears at rest.
