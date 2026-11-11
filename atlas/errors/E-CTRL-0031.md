id: E-CTRL-0031
title: Gains raised and the loop rate left alone
match: stiffer gains need a faster controller, roughly in proportion to one over the square root of kp
match: the largest stable period is a sharp edge: stable just under it, not just over
platforms: linux, windows
teaches: 14-12-the-rate-the-loop-runs-at
---

## Symptom

A stiffer joint was asked for and the loop that held the softer one now oscillates, with no change to the code that runs it.

## Cause

The limit falls as the natural frequency rises. Measured largest stable period:

| kp | kd | period (s) | limit times natural frequency |
|---|---|---|---|
| 25 | 5 | 0.3424 | 1.71 |
| 100 | 10 | 0.1821 | 1.82 |
| 400 | 20 | 0.0950 | 1.90 |

Four times the kp roughly halved the period the loop could survive.

## Fix

Whenever the gains go up, recompute the limit and the rate that follows from it.

```cpp
const double period = period_for(kp, kd, 0.1);
```
