id: E-SAFE-0020
title: A check that trips on noise and is turned off
match: a tight tolerance and no debounce trip on noise, and a debounce and a wider tolerance cost detection time
platforms: linux, windows
teaches: 19-07-two-sensors-that-must-agree
---

## Symptom

The robot stops every few minutes for no reason that anyone can find, and the check is switched off in the field.

## Cause

With noise of 0.02 on each channel, a tolerance of 0.04 and no debounce gave 15797 false trips in 100000 samples, a debounce of 3 gave 303, and 0.12 with a debounce of 3 gave 0. A drift of 0.005 per sample took 3 samples to detect at (0.04, 1) and 29 at (0.12, 3).

## Fix

Set the tolerance from the noise and the debounce from the stopping distance.

```cpp
config.tolerance = 4.0 * sigma_of_difference;
```
