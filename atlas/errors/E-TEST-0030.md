id: E-TEST-0030
title: A suite never shown to catch anything
match: no fault gets past all of the checks
match: expected missed
platforms: linux, windows
teaches: 05-12-six-faults-and-a-matrix
---

## Symptom

The suite is green, and a controller with its sign flipped, or a sensor off by 0.2, is shipped. Every test passed, which is all a green suite says.

## Cause

A test that has never failed has never been shown to be able to. Whether a check would catch a given fault is a question with an answer, and the way to find it is to put the fault in and run the checks.

Six faults were put into a first-order loop: a flipped sign, a sensor reading 0.2 high, an actuator that can deliver only 0.8, the output limiter removed, a measurement 0.7 s late, and a sensor that read zero for 0.6 s. **0** faults got past all three checks, and the clean run passed all three.

## Fix

Keep a list of faults that can be injected, run every check on every fault, and make "a fault nothing catches" a failing test of its own:

```cpp
RC_CHECK(faults_missed(detection_matrix(checks, runs)).empty());
```
