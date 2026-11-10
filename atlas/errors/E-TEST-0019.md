id: E-TEST-0019
title: A behaviour test with no specification behind it
match: a test of the behaviour passes every harmless change and fails every real bug
match: expected harmless_pass
platforms: linux, windows
teaches: 05-08-the-test-that-broke
---

## Symptom

The suite passes a controller that overshoots by a third, or one that never settles, because the test only checks that the response ends near the target and runs without a crash.

## Cause

A behaviour test is only as good as the numbers it checks. The properties a person cares about for a step response are overshoot, how long it takes to settle, and how far from the target it ends. A test that checks one of them lets the others fail.

Measured against five bugs: the integral lost (steady error 0.3333, never settles), gains too low (0.3780, never settles), the integral too strong (overshoot **33.56** percent), gains too high (**18.29** percent), and the sign flipped (diverges). All **5 of 5** failed the specification, and all **6 of 6** harmless changes met it.

## Fix

Check all of them, and a runaway separately, since a value that is not a number fails every comparison quietly:

```cpp
if (m.diverged) return false;
if (m.overshoot_percent > spec.max_overshoot_percent) return false;
if (m.settling_time > spec.max_settling_time) return false;
if (m.steady_error > spec.max_steady_error) return false;
```
