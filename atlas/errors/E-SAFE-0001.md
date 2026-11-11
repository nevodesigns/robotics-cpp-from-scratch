id: E-SAFE-0001
title: A catastrophe classed low because its likelihood was guessed small
match: the classes change at 5, 10 and 15, and a catastrophe is never called low however unlikely
match: the score is severity times likelihood, and out of range gives nothing to rank by
platforms: linux, windows
teaches: 19-01-what-can-go-wrong
---

## Symptom

The risk assessment shows the worst outcomes as green, and the review waves them through, because the likelihood was written as improbable.

## Cause

Severity times likelihood scored a death at likelihood 1 as 4, which is low, level with a scuffed bumper. The likelihood is an estimate made by the people who want the machine to pass, and it is the number that moves most easily. With the floor, a severity of 4 at likelihood 1 is medium (4) and at likelihood 2 is medium (8); severity 3 at likelihood 1 stays low (3).

## Fix

Never class the worst outcome by the product alone.

```cpp
if (severity == 4 && c == RiskClass::low) c = RiskClass::medium;
```
