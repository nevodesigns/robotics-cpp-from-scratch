id: E-FIELD-0035
title: A watchdog that nobody tested
match: the robot kept driving after the link to the computer was cut
match: no watchdog on the drive
match: each broken drive is caught, by the promise it breaks
platforms: linux, windows
teaches: 22-11-the-contract-test-for-an-interface
---

## Symptom

The program crashes and the robot drives on at its last speed.

## Cause

The drive had no watchdog and nothing tested for one. The contract's watchdog promise caught it (1 broken).

## Fix

Make the watchdog a promise in the contract and test every implementation for it.

```cpp
if (since_command_ > limits_.watchdog) target = DriveCommand();
```
