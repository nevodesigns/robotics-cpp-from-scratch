id: E-FIELD-0051
title: A control that nothing proves
match: the safety case says the supervisor stops the robot and no test shows it
match: a control with no test or a test that does not pass
match: a template with nothing filled in has a problem for every hazard, and a filled one has none
match: five deliberate holes are each found, in words
match: a filled case names its controls in the report, and never says none for a hazard that has one
platforms: linux, windows
teaches: 22-17-a-safety-case-per-robot-type
---

## Symptom

The safety case reads well and nobody can say which test shows that a control works. The safety case was written as a document.

## Cause

A control was written down with no test, or with a test that fails.

A control that is described but not tested is a claim. Only a test that exercises it and passes shows that it works, and only a checker that requires the test keeps the case honest as it changes.

## Fix

Make every control name a test and require that test to pass. The checker found 6 problems in a case with five deliberate holes.

```cpp
const std::vector<std::string> problems = rc::field::check_case(c);
```
