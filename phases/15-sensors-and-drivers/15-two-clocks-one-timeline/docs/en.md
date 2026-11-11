# Two Clocks, One Timeline: Putting Streams That Do Not Tick Together on the Same Instants

> The gyro says 100 times a second, the camera says 10, and neither one says when you asked.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 15-03, 15-11

## The Problem

Lesson 15-03 gave each reading the instant it describes, and lesson 15-11 fused two sensors that were conveniently read together. Real sensors are not. An IMU reports a hundred times a second, a camera ten times, a wheel encoder when it feels like it.
To compare a camera's answer with the IMU's, or to fuse them, both have to be asked about the same instant, and that instant belongs to neither.

The easy answer is the newest value each one has sent. That value describes a moment up to a whole sample ago.

## The Concept

### Hold, or draw a line

Two ways to answer for an instant t from a stream that has samples either side of it:

- **Hold:** the newest sample at or before t. Cheap, and always available, and as old as the gap since that sample.
- **Interpolate:** join the sample before and the sample after with a straight line and read it at t.

```cpp
a.value + (b.value - a.value) * (t - a.t) / (b.t - a.t)
```

Both find the samples with a binary search, so a long stream costs no more than a short one. Interpolation needs a sample after t, which a live system does not have yet. It answers for a moment in the past, by one sample, which is the price of the better answer.

### What each costs

A sine of amplitude 1 sampled at 10 Hz and read at 100 instants a second, rms error:

| signal (Hz) | held | predicted | line | predicted |
|---|---|---|---|---|
| 0.25 | 0.0613 | 0.0641 | 0.0016 | 0.0016 |
| 0.50 | 0.1225 | 0.1283 | 0.0064 | 0.0064 |
| 1.00 | 0.2384 | 0.2565 | 0.0253 | 0.0255 |
| 2.00 | 0.4733 | 0.5130 | 0.0985 | 0.1019 |

A held value is wrong by how far the signal moved since the sample, which on average is half a sample: the rms is the signal's rms speed times the sample period over the square root of 3. A line is wrong only by the signal's curvature: its rms
is the rms curvature times the period squared over 2 times the square root of 30. The first is proportional to the signal's frequency and the second to its square, so doubling the frequency doubled the held error and quadrupled the line's, and both fall as the signal is sampled faster.
The line was ten times closer at 1 Hz. See `E-SENSE-0032`.

### Two sensors, one signal

A fast stream at 100 Hz and a slow one at 10 Hz both read the same 1 Hz sine exactly, so any disagreement is the alignment:

| slow stream | disagreement, rms |
|---|---|
| held | 0.2384 (worst 0.5878) |
| interpolated | 0.0253 |

A fusion filter given the held values sees two sensors that disagree by a quarter of the signal's amplitude and believes there is a fault, or an error in the model, and has none. See `E-SENSE-0032`.

### What not to invent

Interpolation can answer for anything between the first and last sample. It must not answer outside that, and it must not answer across a hole:

- **Before the first or after the last sample:** there is nothing to join. The call says no and leaves the output alone. See `E-SENSE-0033`.
- **Across a gap:** two samples a second apart have a line between them, and the line is a guess about a second the sensor never saw. With a hole in a 10 Hz stream from 1.0 s to 1.6 s and a limit of 0.15 s, 192 of 251 instants were answered and none of the 59 inside the hole. With no limit all 251 were,
including those 59, with a straight line through whatever happened. See `E-SENSE-0034`.

## Build It

`hold_at`, `interpolate_at` and `resample` in `exercise/solution.hpp`. The sample type is given.

```
rcpp verify 15-15
```

The signals are exact sines, so the figures above are the same on every run.

## Use It

`rc/sensor/align.hpp` joins `rc::sensor`.

```cpp
const std::vector<Sample> on_imu_instants = resample(camera, imu_times, 0.25);   // refuses gaps over 0.25 s
```

**What this does not do.** It joins with straight lines, which is exact for a signal that is straight between samples and not for one that curves: fast signals need a higher order method, or a model (the observer of lesson 14-10 is one). It assumes each stream is sorted by its sample time, and that the times
are comparable, which means they come from the same clock or have been converted (lesson 15-03). It interpolates a value, so it cannot be used on angles that wrap, or on a count that steps, without care. And it knows nothing about a sensor's own latency: the instant in the sample must be the one the reading describes.

## What Breaks First

- **The newest value used as if it were now.** Two sensors disagree by half a sample's movement. See `E-SENSE-0032`.
- **A value made up outside the stream.** Extrapolation passed off as data. See `E-SENSE-0033`.
- **A line drawn across a hole.** A guess where the sensor saw nothing. See `E-SENSE-0034`.

## Ship It

`rc/sensor/align.hpp` holds `Sample`, `hold_at`, `interpolate_at` and `resample`.

The streams are now on the same instants, ready to go through the same pipeline.
