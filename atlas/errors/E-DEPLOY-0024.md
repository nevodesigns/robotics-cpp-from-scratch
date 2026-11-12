id: E-DEPLOY-0024
title: A back-off that never forgets
match: a back-off that never resets punishes a service that has been healthy for a day, and one that resets after a stable run does not
platforms: linux, windows
teaches: 20-08-boot-into-your-code
---

## Symptom

A service that has run for a day takes thirty seconds to come back after one crash.

## Cause

A service that ran 100 s between crashes, watched for 1000 s, started 9 times with a back-off that never reset, with waits that grew to 30 s, and 10 times with one that reset after a run of 30 s, each with a wait of 1 s.

## Fix

Reset the back-off after a stable run.

```cpp
if (lifetime >= p.stable_after) k = 0;
```
