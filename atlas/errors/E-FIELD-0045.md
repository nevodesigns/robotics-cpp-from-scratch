id: E-FIELD-0045
title: A comparison made at the end only
match: the two simulators agree where the robot stops and disagree on the way
match: only the final pose of the two runs was compared
match: the gap between two traces is the distance, sample against sample
platforms: linux, windows
teaches: 22-15-two-simulators-and-the-gap
---

## Symptom

The final poses agree and the planner behaves differently in the two simulators.

## Cause

Only the end was compared. The way there can differ while the end is the same; the gap has a largest and an rms as well as a final value.

## Fix

Compare the whole trace, sample against sample, and keep all three numbers.

```cpp
const TraceGap g = compare_traces(a, b);  // max_position, rms_position, final_position
```
