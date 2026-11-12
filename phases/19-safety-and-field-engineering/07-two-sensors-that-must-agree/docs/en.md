# Two Sensors That Must Agree: A Cross-Check With a Debounce, What It Costs in False Trips and Late Detection, and What It Cannot Catch

> Two sensors that agree are not necessarily right. Two sensors that disagree are certainly not both right, and that is the only thing the check proves.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (two simulated channels)
**Prerequisites:** 19-05, 19-01, 18-06, 18-07

## The Problem

G8 in lesson 19-01 was "a sensor fails and nothing notices", scored 12. Lesson 19-05 refuses a reading that is not a number, or old. It cannot refuse one that looks fine and is wrong: a wheel encoder that has slipped on its shaft, a range sensor that is stuck at a value, a gyro that has drifted. With one sensor, a wrong answer and a right answer look the same.

With **two**, they do not. The classic answer in safety engineering is redundancy and a comparison: measure it twice, by different means, and treat a disagreement as a failure of one of the two, without needing to know which.

## The Concept

### Agree, suspect, fault

`CrossCheck` takes two readings per sample. They agree if both are valid (a number, inside `min_valid..max_valid`) and differ by no more than the **tolerance**. If they agree the state is `ok` and the value is their mean. If they do not, the state is `suspect` and the value is **the last agreed one**: not the new reading of either, since neither is believed. One suspect sample followed by a good one was forgotten.

A fault needs a whole **debounce** of bad samples in a row. Three in a row latched: the state was `faulted`, the reason `disagree`, and the value was **NaN**. When a sensor is not to be believed there is no value to hand on, and a NaN is the one thing that every later stage has to handle (lesson 19-05 does). And the fault **latches**: the two channels agreeing again did not clear it, only `reset()` did. A pair that disagreed and now agrees has not recovered: it has shown that it can do the first, and somebody has to look. See `E-SAFE-0019`.

A reading that is not a number or is out of range is a bad sample on its own, whatever the other says. The reason names the channel, A before B: a NaN on A gave `invalid_a`, 250 and -3 on B gave `invalid_b`, and both bad gave `invalid_a`.

### What the debounce and the tolerance cost

Two sensors with noise of 0.02 each, reading a true value of 1.0. The difference of two noises has a sigma of 0.028. Healthy samples, 100000 of them, with the check reset after each trip:

| tolerance | debounce | false trips |
|---|---|---|
| 0.04 | 1 | **15797** |
| 0.04 | 3 | 303 |
| 0.08 | 1 | 460 |
| 0.08 | 3 | 0 |
| 0.12 | 1 | 2 |
| 0.12 | 3 | 0 |

A tolerance of 0.04 is 1.4 sigma of the difference, and with no debounce the check trips on one sample in six. A debounce of 3 cuts that by a factor of 52 and a wider tolerance by more. A check that trips on noise is a machine that stops for no reason, and a machine that stops for no reason gets the check removed. That is the way most safety checks die. See `E-SAFE-0020`.

The cost is in the other direction. Sensor B starts to drift away at 0.005 per sample, and the samples until the fault latched (the median of 25 runs with different noise):

| tolerance | debounce | samples to detect |
|---|---|---|
| 0.04 | 1 | 3 |
| 0.04 | 3 | 13 |
| 0.08 | 1 | 11 |
| 0.08 | 3 | 19 |
| 0.12 | 1 | 18 |
| 0.12 | 3 | 29 |

From 3 samples to 29: a factor of ten between the most sensitive and the least. A wider tolerance and a longer debounce are each paid for in the time between the fault and the stop, and **that time is distance** (lesson 19-04). The numbers are a trade-off with no right answer in general: the tolerance comes from the noise (several sigma of the difference), the debounce from how many samples the stopping distance can afford, and neither should be set by looking at which one stops the false trips. See `E-SAFE-0020`.

### What it cannot see

Two channels that are wrong **in the same way** agree. The truth was 3.0 m; both channels read 1.5 m for 50 samples (a shared reference voltage, a shared mounting, two sensors of the same type fooled by the same surface) and the check said `ok`, value 1.5. This is called a **common cause failure**, and it is why the second sensor should differ from the first in the way it measures (a wheel encoder and a visual odometry, a lidar and an ultrasonic sensor, a bumper and a range), in its power and in its wiring. A cross-check proves that the channels agree, which is a different thing from proving that either is right. See `E-SAFE-0021`.

## Build It

`CrossCheck::update` in `exercise/solution.hpp`. The configuration, `Verdict` and `reset` are given.

```
rcpp verify 19-07
```

## Use It

`rc/safety/cross_check.hpp` joins `rc::safety`.

```cpp
rc::safety::CrossConfig config;
config.tolerance = 4.0 * sigma_of_difference;
config.debounce = 3;
rc::safety::CrossCheck check(config);
const rc::safety::Verdict v = check.update(encoder_speed, visual_speed);
if (v.state == rc::safety::CheckState::faulted) trigger_stop();
```

**What this does not do.** It compares two numbers measured in the same units at the same instant: two sensors with different delays disagree during every change, and the tolerance needs to be wider by the speed times the difference in the delays (lesson 15-03). It does not say which sensor is the bad one, and with **three** channels a vote could (a different design, with its own cost). It does not catch a fault that is the same in both, as above, or a fault that is smaller than the tolerance, which is a fault the tolerance has made allowed. A tolerance is a statement of how wrong the robot is permitted to be.

## What Breaks First

- **A fault that is treated as one noisy sample, or a latch that clears itself.** See `E-SAFE-0019`.
- **A check that trips on noise and gets turned off.** See `E-SAFE-0020`.
- **Two sensors with the same weakness.** See `E-SAFE-0021`.

## Ship It

`rc/safety/cross_check.hpp` holds `CheckState`, `CheckReason`, `CrossConfig`, `Verdict` and `CrossCheck`.
