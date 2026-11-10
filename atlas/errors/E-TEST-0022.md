id: E-TEST-0022
title: A bound that is wide enough to be quiet and too wide to see the bug
match: a bound wide enough not to cry wolf still catches a bug, if it is big enough for the readings
match: expected seen_at
platforms: linux, windows
teaches: 05-09-the-bound-you-picked
---

## Symptom

A flaky test is made reliable by widening its bound. It stops failing, and later a real fault, a small offset in a sensor, goes through the suite for months.

## Cause

A wider bound removes false alarms and removes detections by the same amount. How big a bug can be seen depends on the bound, and the bound's width depends on how many readings the test averages.

Measured: with 100 readings and a bound of six spreads (0.6), a bias of 0.3 was seen **0.1** percent of the time, 0.5 in 15.1 percent, 0.7 in 83.5, 0.8 in 97.0 and 1.0 always. The test cannot see a bug of 0.3 at that length.

## Fix

Decide how small a bug the test must see, then take enough readings:

```cpp
const int n = samples_needed(noise, bias, 6.0);      // 1600 readings for a bias of 0.3
```
