id: E-DEPLOY-0030
title: Records sorted by the wall clock
match: sorted by the wall clock, the early records of the second boot go before everything the first boot wrote; sorted by boot and time since boot, they do not
platforms: linux, windows
teaches: 20-10-what-time-it-is-at-boot
---

## Symptom

An incident log shows the robot starting up and then, a second later, a motor fault from the last shift.

## Cause

Six records of two boots sorted by wall time had 5 in the wrong place, the second boot's early records before the first boot's. Sorted by boot and then time since boot all were right, and records with equal times kept their order.

## Fix

Order by boot and time since boot, and keep the wall time to read.

```cpp
sort_records(records);
```
