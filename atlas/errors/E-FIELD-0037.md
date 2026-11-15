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

The two simulators give different results, and the difference is exactly a change someone made in one of them. Someone changed the wheel radius in one model.

## Cause

A dimension was written in two places: the track width, the wheel radius and the limits were typed into each model.

A number typed in two places is two numbers. They agree until someone changes one, and then the two simulators model two different robots.

## Fix

Describe the robot once and make both models from the description.

```cpp
const std::string sdf = rc::field::to_sdf(p.robot);
const rc::field::VehicleLimits lim = rc::field::limits_of(p.robot);
```
