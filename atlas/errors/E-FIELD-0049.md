id: E-FIELD-0049
title: A tick count taken from the data sheet
match: the robot goes 15 percent too far every time
match: the ticks a metre are not the documented ones
match: the numbers measured are the ones the fault set, and the advice says what to do about it
platforms: linux, windows
teaches: 22-16-a-bring-up-checklist-for-a-board
---

## Symptom

Odometry is long or short by the same fraction on every run.

## Cause

The documented counts were not the board's. A tick count 15 percent off read as 1378 ticks a metre against 1200.

## Fix

Drive a measured distance and read the ticks a metre; put the measured value in the driver.

```cpp
const double per_metre = ticks / measured_metres;
```
