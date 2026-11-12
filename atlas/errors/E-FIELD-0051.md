id: E-FIELD-0051
title: A control that nothing proves
match: the safety case says the supervisor stops the robot and no test shows it
match: a control with no test or a test that does not pass
platforms: linux, windows
teaches: 22-17-a-safety-case-per-robot-type
---

## Symptom

The case reads well and nobody can say which test shows that a control works.

## Cause

A control was written down with no test, or with one that fails. The checker found 6 problems in a case with five deliberate holes.

## Fix

Make every control name a test and require the test to pass.

```cpp
if (k.test.empty()) problems.push_back("control " + k.id + " has no test that proves it");
```
