id: E-FIELD-0031
title: A mean time that hides the slow trials
match: the average cycle time is fine and the cell misses its deadline
match: a failed trial counted as a fast one
match: manipulation trials: the mean hides the slow ones and a failed trial does not count as fast
platforms: linux, windows
teaches: 22-10-scoring-a-run
---

## Symptom

The cell meets its average time and misses its deadline one cycle in twenty.

## Cause

The mean hid the slow trials. The mean was 4.72 s and the 95th percentile 9.00 s. A failed trial must not enter the times at all, or it pulls the mean either way.

## Fix

Report the 95th percentile with the mean, and keep failures out of the times.

```cpp
const rc::field::TrialSummary s = rc::field::summarise_trials(trials);
```
