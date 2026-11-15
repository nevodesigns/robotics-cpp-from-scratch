id: E-FIELD-0001
title: A drive with no watchdog keeps driving when the commander is gone
match: a base told to go 0.5 m/s once drives on after its commander has crashed
match: the watchdog stops the base when no command has arrived for its time
match: the same program drives any Drive, and a turn command turns
match: a base that turns as it drives goes round a circle, in y as well as in x
platforms: linux, windows
teaches: 22-01-a-robot-as-an-interface
---

## Symptom

The control program crashes or the link drops, and the robot carries on at its last speed until it hits something.

## Cause

The base held its last command for ever. A drive that only does what it was last told has no way to notice that nobody is telling it anything.

## Fix

Give every drive a watchdog that drops a stale command to a stop, and test it. With a watchdog of 0.5 s, a base at 0.5 m/s was below 0.01 m/s 1.265 s after the commands stopped: the watchdog's time plus the base's own slowing.

```cpp
if (since_command_ > limits_.watchdog) target = DriveCommand();   // zero
```
