id: E-TEST-0021
title: A bound chosen so that the run in front of you passes
match: a bound of one spread, which looked fine on the first run, fails about a third of the time
match: expected this to be true: failures
platforms: linux, windows
teaches: 05-09-the-bound-you-picked
---

## Symptom

A test of a filter on noisy readings is written, run, and passes, with a bound of 0.1 because the result was 0.013. It goes into the suite, and from then on fails on about every third run, always on different machines and seeds. Somebody adds a retry.

## Cause

A quantity worked out from noisy readings is itself a random number. The average of 100 readings with noise of spread 1 has a spread of about 1 / 10, and a bound of 0.1 is one spread: the result falls outside it a third of the time. The run used to pick the bound was one draw from that spread, and a good one.

Measured: over 10000 seeds the average had a mean of 4.9988 and a spread of **0.0997**. The first run (seed 1) passes a bound of 0.1, and **3176 of 10000** runs failed it, 31.8 percent.

## Fix

Measure the spread over many seeds and put the bound a number of spreads wide, not at the value of one run:

```cpp
const Spread s = mean_stddev(averages);
const double bound = bound_for(s.stddev, 6.0);
```
