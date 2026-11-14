id: E-FIELD-0032
title: A gait judged by its average
match: the average stride period is right and the robot stumbles
match: a missed step is hidden in the mean
match: gait metrics: duty factor, stride period and how irregular a stumble is
platforms: linux, windows
teaches: 22-10-scoring-a-run
---

## Symptom

The mean stride period and the duty factor look right and the robot is seen to stumble.

## Cause

One missed step is one long period among many. The irregularity went from 0.000 to 0.222.

## Fix

Score the spread of the periods as well as their mean.

```cpp
const GaitMetrics g = gait_metrics(contact, dt);
```
