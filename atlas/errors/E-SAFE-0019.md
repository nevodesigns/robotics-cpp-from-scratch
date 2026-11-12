id: E-SAFE-0019
title: A disagreement that was treated as noise, or a fault that cleared itself
match: a fault needs the whole debounce in a row, latches, and says there is nothing to believe
match: two channels that agree give their mean, and the first disagreement is only a suspicion
match: a reading that is not a number or is out of range is a bad sample, and the reason names the channel
platforms: linux, windows
teaches: 19-07-two-sensors-that-must-agree
---

## Symptom

A sensor fails, the readings agree again for a moment, and the check is back to green and the robot is on its way.

## Cause

Three bad samples in a row latched a fault with the value NaN; the channels agreeing again did not clear it, and reset() did. One bad sample followed by a good one was forgotten. A NaN on A gave invalid_a, 250 on B gave invalid_b.

## Fix

Latch the fault and hand on NaN.

```cpp
if (bad_run_ >= cfg_.debounce) { faulted_ = true; v.value = NAN; }
```
