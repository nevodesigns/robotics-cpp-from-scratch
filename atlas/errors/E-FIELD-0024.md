id: E-FIELD-0024
title: A run that cannot be compared with another
match: two runs differ and nobody can say where
match: the same seed should give the same log
match: the same seed gives the same run, and another seed differs from the first sample
match: entries are found by time and by channel
platforms: linux, windows
teaches: 22-08-recording-and-replaying-a-run
---

## Symptom

A change makes the robot behave differently and nobody can find when the two runs first parted. Two runs looked alike and ended in different places.

## Cause

There was no way to say when two logs first differ.

Two runs with the same inputs should be identical. If they are not, the first moment they differ is where to look. Without a tool to find it, people compare plots by eye.

## Fix

Compare logs by the time of their first difference, and make determinism a test. The same seed twice gave none; another seed differed from 0.02 s.

```cpp
const double t = rc::field::first_difference(a, b, 1e-9);
```
