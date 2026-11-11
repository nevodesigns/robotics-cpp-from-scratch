id: E-CTRL-0033
title: Gains tuned at one pose used at all of them
match: gains tuned for one pose are wrong at the others, and worst at the heaviest
match: gains for a joint come from its inertia: kp is J w squared and kd is 2 zeta J w
platforms: linux, windows
teaches: 14-13-gains-that-follow-the-pose
---

## Symptom

The joint is crisp when the arm is folded and rings or crawls when it is reaching.

## Cause

A joint's natural frequency is the square root of kp / J and its damping ratio is kd over twice the square root of kp J. When J grows, both fall. Gains tuned folded (kp 32, kd 4.8), measured:

| extension | inertia | overshoot | settling (s) |
|---|---|---|---|
| 0.00 | 0.50 | 0.087 | 0.74 |
| 0.50 | 1.12 | 0.245 | 1.57 |
| 1.00 | 3.00 | 0.444 | 4.32 |

## Fix

Make the gains follow the inertia.

```cpp
const Gains g = scheduled_gains(inertia, 8.0, 0.6);   // kp = J w^2, kd = 2 zeta J w
```
