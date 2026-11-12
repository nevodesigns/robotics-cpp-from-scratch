id: E-DEPLOY-0035
title: A trial shorter than the failure
match: a release that crashes is put on trial and taken back by the robot, and one that fails only after the soak is not, which is the limit of the trial
match: a release for the wrong machine, a damaged one, an older one and an oversize one are each refused before the disk is touched
platforms: linux, windows
teaches: 20-12-a-release-that-reaches-the-robot
---

## Symptom

The release passed the trial and the robot crash loops an hour later, with no way back.

## Cause

A release crashing at 2 s was rolled back to 1.4.2 and was up. One healthy for 90 s that then crashed was confirmed at 50 s with a soak of 30 s: running 1.5.0, confirmed, not up. Eight refusals left the disk untouched.

## Fix

Choose the soak from how long the failures take to show, and check everything that changes nothing first.

```cpp
const auto o = deploy(robot, release, -1, false);
```
