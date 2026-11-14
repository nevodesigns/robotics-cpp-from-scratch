id: E-FIELD-0052
title: A high risk controlled by a sign
match: a hazard that can kill is controlled by a warning label
match: the only control against a high risk is information
match: the risk level is the product of severity, exposure and avoidability
match: each robot type starts from its own hazards, with unique ids and some of them high
match: five deliberate holes are each found, in words
platforms: linux, windows
teaches: 22-17-a-safety-case-per-robot-type
---

## Symptom

The worst hazards in the case are controlled by a notice in the manual.

## Cause

A control of kind information was accepted against a high risk. It relies on a person doing the right thing every time.

## Fix

Require a design or a protective control against every high risk, with information in addition.

```cpp
if (risk_of(h) == "high" && protective == 0) problems.push_back(...);
```
