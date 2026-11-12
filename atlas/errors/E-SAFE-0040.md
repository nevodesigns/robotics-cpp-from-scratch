id: E-SAFE-0040
title: A mitigation that is a promise
match: the ground vehicle's eight hazards, each with mechanisms that exist and evidence that passes, are defensible, and the residual risk is worked out
match: a mitigation that is only a name, a second one in the same group, a missing acceptor, an unaddressed hazard and a pfd nobody knew are each caught
platforms: linux, windows
teaches: 19-14-a-risk-assessment-you-can-defend
---

## Symptom

The assessment lists an independent watchdog, and nobody can say where it is, or whether it was ever switched on.

## Cause

The eight hazards, with evidence run against the real mechanisms, gave 0 findings and eight medium residuals from one unacceptable, three high and four medium. A mitigation naming a mechanism that does not exist was a finding, unknown mechanism: magic watchdog v2, and earned no credit.

## Fix

Make each mitigation name a mechanism that exists and carry evidence that runs.

```cpp
mit(id, mechanism, group, pfd, evidence);
```
