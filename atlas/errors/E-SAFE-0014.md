id: E-SAFE-0014
title: A NaN that went through a min() at full speed
match: the obvious way of using a range lets a reading that is not a number through at full speed
platforms: linux, windows
teaches: 19-05-a-speed-the-sensor-can-vouch-for
---

## Symptom

A division by zero in a driver, or an unset field, and the speed limit stops limiting, and nothing logs it.

## Cause

std::min(2.0, max_speed(NaN, ...)) returned 2.0, because a comparison with NaN is false and the first argument wins; infinity gave 2.0 as well. The limiter gave 0 for NaN and under 1.5 m/s for no return.

## Fix

Check for NaN before any min or max, and treat it as the unsafe case.

```cpp
if (std::isnan(command) || command == 0.0) return 0.0;
```
