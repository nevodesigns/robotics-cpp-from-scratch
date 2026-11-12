id: E-DEPLOY-0014
title: A commit that was not last
match: the commit is the hash and the move of the pointer is last: a cut between them boots the old release, and only the whole boots the new
match: an install that was cut can simply be run again, from the start, and finishes whatever step it was cut at
platforms: linux, windows
teaches: 20-05-a-power-cut-at-every-step
---

## Symptom

After a failed update the robot runs a half written release, or the update has to be repaired by hand.

## Cause

With the two slot plan of ten steps, nine steps left release 2 complete and valid in slot 1 and the robot booting release 1; the tenth moved the pointer, and release 1 was still in slot 0. Cut at each of 11 points and run again, 11 of 11 ended on release 2.

## Fix

Write the chunks, then the hash, then the pointer, and plan from what boots.

```cpp
const int target = booted.ok ? 1 - booted.slot : 0;
```
