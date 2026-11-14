id: E-FIELD-0023
title: A log written with too few digits
match: two logs of the same run differ in the sixth digit
match: the replay is off by a small amount after the round trip
match: a recorded run reads back exactly, scenario and all, and with too few digits it does not
platforms: linux, windows
teaches: 22-08-recording-and-replaying-a-run
---

## Symptom

A replay does not reproduce the run exactly, and a comparison of two runs shows differences that are not real.

## Cause

The values were written with the default 6 digits. After the round trip they were off by up to 0.0000050; with 17 digits they were identical.

## Fix

Write doubles with 17 digits, and test that a log reads back exactly.

```cpp
o << std::setprecision(17);
```
