id: E-FIELD-0045
title: A comparison made at the end only
match: the two simulators agree where the robot stops and disagree on the way
match: only the final pose of the two runs was compared
match: the gap between two traces is the distance, sample against sample
platforms: linux, windows
teaches: 22-15-two-simulators-and-the-gap
---

## Symptom

The final poses agree, and the planner behaves differently in the two simulators. The comparison printed only the final pose.

## Cause

Only the end was compared. The way there can differ while the end is the same.

Two traces can wander apart and come back together. The final gap is small; the largest is not. A planner sees the way, not only the end.

## Fix

Compare the whole trace sample against sample, and keep the largest gap and the rms as well as the final one.

```cpp
const rc::field::TraceGap g = rc::field::compare_traces(a, b);   // max_position, rms_position, final_position
```
