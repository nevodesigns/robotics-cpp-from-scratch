id: E-DEPLOY-0018
title: A soak shorter than the time a failure takes to show
match: a soak shorter than the time a failure takes to show confirms a release that is bad, and the soak is what has to be chosen
match: a release whose checks pass after the deadline is treated as one that never passed
platforms: linux, windows
teaches: 20-06-on-trial-until-it-proves-itself
---

## Symptom

The release is confirmed, runs well for an hour, and crashes, and now there is nothing to go back to.

## Cause

A release healthy at 20 s that crashes at 120 s was confirmed and looped for 20 boots with soaks of 0, 30 and 90 s, and was rolled back in 4 boots with 100 s and 150 s. Healthy at 90 s against a deadline of 60 s was rolled back.

## Fix

Choose the soak from how long the failures take to show.

```cpp
confirm_after = time_to_first_failure_seen + margin;
```
