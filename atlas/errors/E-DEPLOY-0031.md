id: E-DEPLOY-0031
title: A profile from a test too short to see the peak
match: a minute of measuring on the bench misses the peak that an hour in the field has, and the planner's peak looks a fifth of its size
match: a percentile is the nearest rank, and the profile keeps the average, the peak and the largest memory
platforms: linux, windows
teaches: 20-11-does-it-fit
---

## Symptom

The robot passed its resource check and misses deadlines in the field, in the one moment the planner replans.

## Cause

A planner at 0.4 cores that peaks at 2.0 once in 70 s had an average of 0.40 and a peak of 0.40 over a bench run of 60 s, and an average of 0.42 and a peak of 2.00 over an hour in the field, a fifth of the real peak.

## Fix

Measure for as long as the longest cycle, and keep the largest sample.

```cpp
profile_from_samples(name, cpu, ram, 1.0);
```
