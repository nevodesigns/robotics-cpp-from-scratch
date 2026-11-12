id: E-SAFE-0032
title: A test of the software that leaves the machine untested
match: a check that only reads the software's answer passes with the contactor welded, and one that asks the contactor does not
match: permission to drive needs a test that passed, lasts an interval, and is lost by a failed test or a clock that goes back
platforms: linux, windows
teaches: 19-11-prove-it-still-works
---

## Symptom

The self-test passes every morning, and the robot cannot stop when a person steps in front of it, because the contactor is welded.

## Cause

With a contactor welded closed, the check of the software's command passed and the check that commanded the contactor open and read its auxiliary contact failed; with a healthy one both passed. A permit of 3600 s after a pass at 100 s held up to 3700 s and not at 3700.5 s, and a failed test at 2000 s cancelled it.

## Fix

Test the real path to the last thing that can be read back, and let a permit expire.

```cpp
c.command(false);
return !c.closed();
```
