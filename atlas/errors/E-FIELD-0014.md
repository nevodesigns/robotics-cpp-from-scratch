id: E-FIELD-0014
title: A slope treated as the same in every direction
match: the vehicle climbs a slope sideways but not up it
match: the grade along the heading is not the steepest grade
match: a slope helps one way and hurts the other, and costs nothing across it
platforms: linux, windows
teaches: 22-05-terrain-and-friction
---

## Symptom

The same slope is easy on one heading and impossible on another, and a planner that ignores heading cannot tell. The slope is steep in one direction and gentle in the other.

## Cause

The slope was treated as the same in every direction. What matters is the slope along the heading: on a grade of 0.2 the drive could give 2.020 m/s2 up it and 4.022 across it.

A cell's slope has a direction. The work to climb it depends on the gradient along the way the vehicle goes, which is the cell's gradient projected on the heading.

## Fix

Take the gradient along the heading, and plan with the heading.

```cpp
const double grade = c.grade_x * std::cos(heading) + c.grade_y * std::sin(heading);
```
