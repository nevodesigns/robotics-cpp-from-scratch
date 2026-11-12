id: E-DEPLOY-0034
title: A downgrade that the robot accepts
match: versions are three numbers, compared by major then minor then patch, and the rules of whether to take one are checked in order
match: a good release goes through every stage, is booted from the other slot, and confirms itself
platforms: linux, windows
teaches: 20-12-a-release-that-reaches-the-robot
---

## Symptom

A robot is made to run an old release with a known fault, by being offered it, or a version is sorted wrongly and the older is taken for the newer.

## Cause

1.10.0 was newer than 1.9.0 by number and sorted before it as text. A rollback to 1.2.0 with a floor of 1.3.0 was refused even when authorised; one to 1.4.0 was installed when authorised. A good release passed target, manifest, version, fit, install and trial and the old release stayed valid in slot 0.

## Fix

Compare numbers, keep a floor and refuse a downgrade.

```cpp
if (compare(offered, minimum) < 0) return VersionDecision::refuse_below_minimum;
```
