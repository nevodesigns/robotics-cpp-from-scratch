id: E-DEPLOY-0021
title: Going on because nobody complained
match: when too few robots report, there is no news and the rollout stops, and one that goes on regardless sends the release blind
platforms: linux, windows
teaches: 20-07-a-few-robots-first
---

## Symptom

The rollout shows no failures, and the robots that crashed into the update are the ones that cannot say so.

## Cause

With 45 percent of a fleet of 1000 silent, a rollout that halted below 80 percent reporting stopped at wave 0 after 10 robots, 4 of them silent; one that did not check gave the release to all 1000, 450 of them silent.

## Fix

Halt when too few robots report.

```cpp
policy.min_reported_ratio = 0.8;
```
