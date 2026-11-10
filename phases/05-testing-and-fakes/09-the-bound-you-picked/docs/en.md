# The Bound You Picked: Tolerances for a Test With Noise in It, and How Many Samples It Takes to See a Bug

> A test of a filter on noisy readings needs a bound, and a bound taken from the run in front of you
> fails a third of the time. The right one comes from a spread you measure, and how small a bug it can see
> comes from how many readings you take.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-12, 02-08, 05-05

## The Problem

A test averages a hundred noisy readings and checks that the result is near the truth. It is run, the error is 0.013, and a bound of 0.1 goes in. From then on the test fails on about one run in three.

Lesson 05-05 found tests that fail sometimes for reasons in the program. This one fails sometimes for a reason in the test: what it checks is itself a random number, and the bound was chosen without asking how much that number varies.

## The Concept

### The statistic has a spread

The average of 100 readings, each with noise of spread 1, varies from seed to seed by about 1 / sqrt(100) = 0.1. Over 10000 seeds the suite saw a mean of 4.9988 and a spread of **0.0997**. That, and not the noise on a reading, is what the bound has to fit. `mean_stddev` measures a mean and a spread from many runs.

### One spread

The first run (seed 1) passes a bound of 0.1. Over 10000 seeds **3176** failed it: **31.8 percent**. The run used to choose the bound was one draw from the spread, and a lucky one. See `E-TEST-0021`.

### How many spreads

| bound | spreads | failures in 10000 |
|---|---|---|
| 0.1 | 1 | 3176 |
| 0.3 | 3 | 26 |
| 0.6 | 6 | 0 |

Three spreads fails about once in 400, which looks perfect when run a few times and means a suite of three thousand such tests fails nearly every run. Six spreads is about one in five hundred million. The only way to know a test's false alarm rate is to run it on many seeds and count, which is what `count_failures` does. See `E-TEST-0023`.

### What a wide bound cannot see

A wider bound removes false alarms and detections alike. With 100 readings and a bound of 0.6, a bias, which is a bug, was seen:

| bias | seen |
|---|---|
| 0.3 | 0.1 percent |
| 0.5 | 15.1 percent |
| 0.7 | 83.5 percent |
| 0.8 | 97.0 percent |
| 1.0 | 100 percent |

A test with a bound of six spreads cannot see a bug of 0.3 at that length. See `E-TEST-0022`.

### Taking enough readings

The average of n readings has a spread of noise / sqrt(n), so more readings narrow the bound. `samples_needed(noise, bias, k)` gives the n for a bias to sit twice as far out as a bound of k spreads: (2 k noise / bias) squared, rounded up.

| noise | bias | k | readings |
|---|---|---|---|
| 1 | 0.3 | 6 | 1600 |
| 1 | 1.0 | 6 | 144 |
| 1 | 0.1 | 6 | 14400 |

With 1600 readings the bound was 0.150, a bias of 0.3 was seen in **2000 of 2000** runs, and there were **0** false alarms in 10000. Twice the noise needs four times the readings.

## Build It

`mean_stddev`, `bound_for`, `samples_needed` and `count_failures` in `exercise/solution.hpp`. The noisy statistic and the seeded generator (lesson 00-12) are given.

```
rcpp verify 05-09
```

## Use It

`rc/test/tolerance.hpp` joins `rc::test`.

```cpp
const int n = samples_needed(noise, smallest_bug, 6.0);
const bool fine = count_failures(my_test, 1, 10000) == 0;
```

**What this does not do.** It treats the statistic as roughly bell shaped, which an average of many readings is and a maximum or a count of rare events is not. Six spreads is a bound for that shape, and a different shape needs its own measurement. It tests one number at a time: a suite that checks
many numbers needs a smaller false alarm rate for each. And it costs run time: 1600 readings for each of ten thousand seeds is sixteen million readings, so a test like this is run once to choose the bound and not on every build.

## What Breaks First

- **A bound taken from one run.** It fails a third of the time. See `E-TEST-0021`.
- **A bound widened until it is quiet.** It stops seeing the bug. See `E-TEST-0022`.
- **A false alarm rate nobody measured.** A large suite fails every run. See `E-TEST-0023`.

## Ship It

`rc/test/tolerance.hpp` holds `noisy_average`, `Spread`, `mean_stddev`, `bound_for`, `samples_needed` and `count_failures`.

Lesson 05-08 asked what a test of a moving thing should say. This asks how wide to make it when the thing has noise in it.
