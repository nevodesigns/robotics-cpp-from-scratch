id: E-TEST-0018
title: A test that pins a value the system was never promised to produce
match: a test that pins an exact value breaks on changes that do not matter
match: expected broken
platforms: linux, windows
teaches: 05-08-the-test-that-broke
---

## Symptom

A change that makes the robot no better and no worse, a different time step, a refactor, a plant constant corrected by five percent, turns the suite red. Somebody updates the expected number to make it pass, and the suite now guards nothing but the current arithmetic.

## Cause

The expected number was written down from the first version that worked. It is the value of an accident: the sum of every small choice that went into that run. The requirement was never that the response at one second is 0.933636. It was that the robot settles in a few seconds without overshooting.

Measured: the value at one second was **0.933636**. With the time step halved it was 0.933610, doubled 0.933686, the plant 5 percent slower 0.936651 and so on: **6 of 6** harmless changes broke the exact test.

## Fix

Assert what was required, as numbers with room in them:

```cpp
Metrics m = measure(simulate(kp, ki, tau, dt, 10.0));
RC_CHECK(meets(Spec(), m));
```
