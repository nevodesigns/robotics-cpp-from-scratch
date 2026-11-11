id: E-SENSE-0032
title: The newest value used as if it were now
match: a held value lags by half a sample on average, and a line is far closer, by the amounts the maths gives
match: two sensors on the same quantity disagree by the hold's lag, and agree once the slow one is put on the fast one's instants
platforms: linux, windows
teaches: 15-15-two-clocks-one-timeline
---

## Symptom

Two sensors reading the same thing disagree in a steady pattern that follows the signal's speed, and a fusion filter reports a fault that is not there.

## Cause

The newest value describes a moment up to one sample ago. For a sine of amplitude 1 at 1 Hz sampled at 10 Hz, rms error of a held value was 0.2384 (predicted 0.2565) and of a line between the samples 0.0253 (predicted 0.0255). Two streams reading the same signal exactly disagreed by 0.2384 rms, worst 0.5878, with the slow one held.

## Fix

Read the slow stream at the fast one's instants by interpolation.

```cpp
const std::vector<Sample> aligned = resample(slow, instants, 1.0);
```
