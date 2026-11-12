id: E-SAFE-0042
title: Credit for redundancy that is not independent
match: within a group a second mitigation adds nothing, across groups two are redundant with a common cause, and an unknown pfd counts as certain failure
platforms: linux, windows
teaches: 19-14-a-risk-assessment-you-can-defend
---

## Symptom

The assessment shows two mitigations for a hazard and a thousandfold improvement, and both fail on the same wet day.

## Cause

One software function gave 3.60e-03 and two in the same group gave 3.60e-03. Software with a hardware brake gave 1.03e-05 at beta 0.10 and 3.60e-07 at beta 0. A second software mitigation for G7 left its pfd at 3.64e-03.

## Fix

Count only the best of each group, and combine groups with a common-cause share.

```cpp
return std::min(1.0, product + b * lowest);
```
