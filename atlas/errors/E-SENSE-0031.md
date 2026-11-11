id: E-SENSE-0031
title: A number of retries chosen by feel
match: with failures at random, the fraction that never gets through is the failure rate to the power of the attempts
match: how many attempts are enough comes from the failure rate and the target, and some targets cannot be met
platforms: linux, windows
teaches: 15-13-the-read-that-fails
---

## Symptom

Readings are lost in bursts at one site and never at another, with the same driver.

## Cause

The chance of giving up is p to the power N. At p of 0.28 the measured figures were 0.2757, 0.0771, 0.0223, 0.0062 and 0.0015 for 1 to 5 attempts. For 99 percent success, p of 0.10 needs 2 attempts, 0.28 needs 4, 0.50 needs 7 and 0.90 needs 44.

## Fix

Measure the failure rate and work out the attempts.

```cpp
const int n = attempts_needed(0.28, 0.99);   // 4
```
