id: E-DEPLOY-0020
title: A canary too small to see a rare fault
match: a fault on one robot in two hundred is not seen by a canary, and a threshold of five percent never halts for it
match: the arithmetic of a wave, and what cannot happen
platforms: linux, windows
teaches: 20-07-a-few-robots-first
---

## Symptom

The staged rollout finishes without a halt and the fault is found by customers, in the fleet's last wave.

## Cause

A fault on 5 of 1000 robots never halted a threshold of 5 percent and reached all 1000; a threshold of 0 halted at wave 2 after 500 robots. The chance that 10 robots include one is 4.9 percent, 100 robots 39.4, 500 91.8, 598 95.0, 1000 99.3; for 95 percent 598 robots are needed and for 99 percent 919.

## Fix

Work out how many robots a wave needs, and test the variants before the release.

```cpp
const int need = robots_needed(0.005, 0.95);
```
