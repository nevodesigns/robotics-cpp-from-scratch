id: E-DEPLOY-0013
title: The release that was running was overwritten
match: overwriting the running release leaves it bricked at every power cut in the middle, and A/B never does
match: a slot is valid when its hash matches what it holds, and the robot boots the pointer's slot, then the other, then nothing
platforms: linux, windows
teaches: 20-05-a-power-cut-at-every-step
---

## Symptom

A robot that lost power during an update does not boot, and has to be taken apart or sent back.

## Cause

Installing an 8 chunk release in place and cutting the power after each of the 10 possible numbers of steps: 1 booted release 1, 1 booted release 2 and 8 were bricked, each holding chunks of both releases and the old hash.

## Fix

Write the slot that is not running.

```cpp
const auto steps = plan_ab(disk, image, version);
```
