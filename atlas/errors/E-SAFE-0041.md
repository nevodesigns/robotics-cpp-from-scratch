id: E-SAFE-0041
title: A mitigation that was true before the last change
match: take the hardware brake away from G2, and the assessment says it needs two independent groups and that the residual risk is high
match: a brake that is applied by power fails its evidence in every hazard that relied on it, and the credit goes with it
platforms: linux, windows
teaches: 19-14-a-risk-assessment-you-can-defend
---

## Symptom

The assessment was right when it was written, and a later design change made one of its claims untrue, and nobody noticed.

## Cause

Without G2's hardware brake the assessment gave two findings and a residual of high (7.28e-02 a year). A brake applied by power failed its evidence in G1, G2 and G4: 3 evidence failures and 9 findings in all.

## Fix

Run the evidence in continuous integration.

```cpp
return assess(entries, known, 0.10).defensible ? 0 : 1;
```
