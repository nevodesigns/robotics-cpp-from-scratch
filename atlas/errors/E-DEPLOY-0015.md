id: E-DEPLOY-0015
title: A robot with nowhere to fall back to
match: release after release alternates the slots, a bad image falls back to the one before, and a robot with no good slot reports it
platforms: linux, windows
teaches: 20-05-a-power-cut-at-every-step
---

## Symptom

One damaged block in the new release and the robot does not start, though the old release is still on it.

## Cause

Releases 2, 3, 4 and 5 used slots 1, 0, 1, 0. A chunk of release 2 damaged after the switch made the robot boot release 1 from slot 0; with release 1 damaged too nothing booted, which was reported, and a recovery install went to slot 0.

## Fix

Keep the last release valid and let the boot fall back.

```cpp
const int order[2] = {first, 1 - first};
```
