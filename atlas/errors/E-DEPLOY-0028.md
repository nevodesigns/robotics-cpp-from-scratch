id: E-DEPLOY-0028
title: Wall time used before it was set
match: a clock with no battery counts from 1970, says it has not been set, and is corrected once the network gives the time
match: a certificate cannot be judged before the time is known, and is judged properly after
platforms: linux, windows
teaches: 20-10-what-time-it-is-at-boot
---

## Symptom

Every boot starts with a connection that fails, and records dated 1970, and then it all works after a minute.

## Cause

A stamp at 12 s read 12 unsynced; once the clock was set at 40 s that moment was known to be 1794699972. A certificate was unknown before the set (the 1970 clock would call it not yet valid) and valid, not yet or expired after.

## Fix

Carry a synced flag and refuse to judge before it.

```cpp
if (!clock.synced()) return Validity::unknown;
```
