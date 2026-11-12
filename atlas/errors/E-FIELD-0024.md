id: E-FIELD-0024
title: A run that cannot be compared with another
match: two runs differ and nobody can say where
match: the same seed should give the same log
platforms: linux, windows
teaches: 22-08-recording-and-replaying-a-run
---

## Symptom

A change makes the robot behave differently and nobody can find when the two runs first parted.

## Cause

There was no way to say when two logs first differ. With one, the same seed twice gave none and another seed differed from 0.02 s.

## Fix

Compare logs by the time of the first difference, and make determinism a test.

```cpp
const double t = first_difference(a, b, 1e-9);
```
