id: E-FIELD-0001
title: A drive with no watchdog keeps driving when the commander is gone
match: a base told to go 0.5 m/s once drives on after its commander has crashed
match: the watchdog stops the base when no command has arrived for its time
match: the same program drives any Drive, and a turn command turns
platforms: linux, windows
teaches: 22-01-a-robot-as-an-interface
---

## Symptom

The control program crashes or the link drops and the robot carries on at its last speed until something hits.

## Cause

The base held its last command for ever. With a watchdog of 0.5 s a 0.5 m/s base was below 0.01 m/s after 1.265 s.

## Fix

Give every drive a watchdog that drops the command when it is stale, and test it.

```cpp
if (since_command_ > limits_.watchdog) target = DriveCommand();
```
