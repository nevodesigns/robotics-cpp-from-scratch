id: E-CTRL-0037
title: A relay margin that is too wide
match: a wide margin on the relay bends the answer, and the gain it reports falls
platforms: linux, windows
teaches: 14-14-a-loop-you-tune-by-experiment
---

## Symptom

The same plant gives a different ultimate gain every time the margin is changed, and the gains made from it are far too gentle.

## Cause

The margin delays every switch, so the swing is bigger and slower than the plant's own. The amplitude is in the denominator of the ultimate gain. Measured:

| margin | amplitude | ultimate gain | period |
|---|---|---|---|
| 0.01 | 0.2231 | 5.71 | 0.544 |
| 0.10 | 0.3627 | 3.51 | 0.692 |
| 0.20 | 0.4820 | 2.64 | 0.804 |

## Fix

Set the margin above the sensor's noise and no more than a small fraction of the swing.

```cpp
const RelayResult found = relay_test(1.0, 0.01);
```
