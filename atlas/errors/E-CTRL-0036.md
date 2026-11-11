id: E-CTRL-0036
title: The relay's numbers taken as exact
match: what the relay finds is the plant's ultimate gain and period, worked out here from its constants
match: the relay makes the speed swing by itself, and reports the swing it saw
platforms: linux, windows
teaches: 14-14-a-loop-you-tune-by-experiment
---

## Symptom

Gains from a relay test and a rule are close and not right: the loop rings a little more or a little less than the rule promised.

## Cause

The relay's output is a square wave, and Ku = 4 d / (pi a) treats it as if it were only its fundamental sine. The other harmonics leave an error. Measured against the values worked out from the plant's constants:

| | relay | worked out |
|---|---|---|
| ultimate gain | 5.71 | 6.42 |
| ultimate period | 0.544 s | 0.513 s |

## Fix

Use the relay for a starting point and verify with a step.

```cpp
const RelayResult found = relay_test(1.0, 0.01);
```
