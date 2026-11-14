id: E-FIELD-0037
title: A dimension written in two places
match: the SDF and the built in model disagree about the track width
match: the robot in one simulator is not the robot in the other
match: a number with no source, a given twice, an unknown key and a missing one are all refused
match: the same file gives the built in limits and the SDF, with the same numbers in both
platforms: linux, windows
teaches: 22-12-describing-a-robot-once
---

## Symptom

The two simulators give different results and the difference is exactly the change someone made in one of them.

## Cause

The track width, the wheel radius and the limits were typed in the model of each. One file now gives both.

## Fix

Describe the robot once and make both models from the description.

```cpp
const std::string sdf = to_sdf(p.robot);
```
