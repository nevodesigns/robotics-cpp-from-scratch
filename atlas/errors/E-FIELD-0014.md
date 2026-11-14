id: E-FIELD-0014
title: A slope treated as the same in every direction
match: the vehicle climbs a slope sideways but not up it
match: the grade along the heading is not the steepest grade
match: a slope helps one way and hurts the other, and costs nothing across it
platforms: linux, windows
teaches: 22-05-terrain-and-friction
---

## Symptom

The same slope is easy on one heading and impossible on another, and a planner that ignores heading cannot tell.

## Cause

The slope that matters is the one along the heading. On a grade of 0.2 the drive was 2.020 m/s2 up it and 4.022 across it.

## Fix

Take the gradient along the heading, and plan with the heading.

```cpp
const double grade = c.grade_x * std::cos(heading) + c.grade_y * std::sin(heading);
```
