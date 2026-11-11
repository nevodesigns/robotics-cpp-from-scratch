id: E-CTRL-0038
title: Rule of thumb gains used without a step test
match: gains from one relay test tune the loop: four numbers for each rule set, measured on a step
platforms: linux, windows
teaches: 14-14-a-loop-you-tune-by-experiment
---

## Symptom

Gains from a published rule are installed and the joint overshoots more than anyone wanted.

## Cause

A rule gives gains that suit a typical plant and a typical goal. From Ku 5.71 and Tu 0.544, measured on a step to 1:

| rule set | rise (s) | overshoot | settling (s) | final error |
|---|---|---|---|---|
| Ziegler-Nichols | 0.135 | 0.597 | 1.574 | 0.0000 |
| some overshoot | 0.226 | 0.376 | 1.943 | 0.0000 |

## Fix

Run a step and read the four numbers before the gains go on the machine.

```cpp
const PidGains gains = some_overshoot(found.ultimate_gain, found.period);
```
