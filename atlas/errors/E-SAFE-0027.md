id: E-SAFE-0027
title: A tilt stop on one bump, or a NaN let through
match: the monitor warns early, stops only on a run of samples past the stop level, and stops at once on a reading that is not a number
platforms: linux, windows
teaches: 19-09-the-slope-and-the-turn
---

## Symptom

The robot stops dead on a stone, or carries on tipping because the tilt reading was a NaN and every comparison with it was false.

## Cause

With a warning at 15 degrees, a stop at 25 and a debounce of 3, one sample at 30 degrees was a warning, 10 degrees after it cleared the count, three in a row (the last on the other side) were a stop, and a NaN was a stop at once.

## Fix

Debounce the stop, and treat NaN as a stop before comparing.

```cpp
if (std::isnan(angle)) return TiltState::stop;
```
