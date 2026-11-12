id: E-SAFE-0029
title: A stop that clears itself, or a reset that forgets a cause
match: a stop latches, every cause seen is kept, and the stop does not clear when the cause does
match: a warning slows the robot, a derated battery slows it by its share, and the range caps it
platforms: linux, windows
teaches: 19-10-one-place-that-says-stop
---

## Symptom

The robot starts again as soon as the tilt clears, or an operator resets a stop for a lost link and the robot drives off with the tilt that was also there.

## Cause

A stop for a tilt followed by a lost link had both on record (link, tilt), stayed stopped when both cleared, refused a reset with a cause present and changed nothing, and was normal with the causes none after a reset with all clear. A tilt warning and a power share of 0.2 gave a cap of 0.4 m/s.

## Fix

Latch, accumulate, and refuse a reset while any cause is present.

```cpp
seen_ |= now;
if (stop_causes(in) != 0) return false;
```
