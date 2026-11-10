id: E-TEST-0020
title: A specification so tight it is the pinned value again
match: the numbers of the specification can be tightened until it fails a change it should not
match: expected failed
platforms: linux, windows
teaches: 05-08-the-test-that-broke-when-nothing-was-wrong
---

## Symptom

After a behaviour test is written it is tightened, to be safe: "settles in 1.8 seconds", from a run that settled in 1.865. A month later a harmless change fails it, and the limit is loosened by the smallest amount that makes it pass.

## Cause

A bound copied from the current result has no room in it. It is the pinned value with one more digit of tolerance. A bound should come from the requirement, which is what the robot has to do, and not from what it happens to do today.

Measured: with the settling time cut to 1.8 s, **4 of 6** harmless changes failed, though none had made the robot worse. The base design settles in 1.865 s against a requirement of 3 s.

## Fix

Take the limit from the need, not from the last run, and keep a margin for the changes that are meant to be harmless:

```cpp
Spec spec;                       // 10 percent, 3 seconds, 0.01: what the application needs
```
