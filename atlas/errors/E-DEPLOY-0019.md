id: E-DEPLOY-0019
title: A release given to the whole fleet at once
match: the waves take the next robots in order, an empty wave is skipped, and a clean fleet goes all the way
match: a fault on three robots in ten is stopped by the first wave, where giving it to everyone fails three hundred
platforms: linux, windows
teaches: 20-07-a-few-robots-first
---

## Symptom

A release that works on every robot in the lab fails on a third of the customers' robots in the same hour.

## Cause

A clean fleet of 1000 went through waves of 10, 90, 400 and 500. A release failing on 300 of 1000 robots failed 300 when given to all at once and 4 of the first 10 when staged, and was halted at wave 0.

## Fix

Roll out in waves with a check between them.

```cpp
policy.cumulative = {0.01, 0.1, 0.5, 1.0};
```
