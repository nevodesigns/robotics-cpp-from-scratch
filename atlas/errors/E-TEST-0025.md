id: E-TEST-0025
title: A fake that accepts what the real hardware would refuse
match: the fake counts a speed set while disabled and a speed past the limit, and does not obey either
match: expected port.violations
platforms: linux, windows
teaches: 05-10-checking-how-not-what
---

## Symptom

The tests pass and the robot on the bench does nothing, because the program set a speed before enabling the driver, which the hardware ignores. Or it asked for more than the limit and the driver refused. The fake had taken both without a word.

## Cause

A fake that stores whatever it is given has no rules, so a program that breaks the rules of the real device passes against it. The rules of the hardware are part of what a test needs to check.

The fake here keeps the state of a motor and counts the violations: a speed set while disabled (1), then a speed of 1.5 over a limit of 1.0 (2), and in neither case did the speed change. A speed of -1.0, exactly at the limit and going backwards, was fine and was not counted.

## Fix

Give the fake the rules, and a counter a test can read:

```cpp
if (!enabled_) { ++violations_; return; }
if (std::fabs(speed) > limit_) { ++violations_; return; }
speed_ = speed;
```
