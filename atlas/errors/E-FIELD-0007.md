id: E-FIELD-0007
title: A limit written in the wrong unit
match: a speed limit of 5 was meant in km/h and used as m/s
match: the stopping distance is many times longer than the plan says
match: a unit is part of the type, and the conversions are right
match: speed up slowly, brake harder, and the numbers are the ones the limits say
match: a limit read in the wrong unit makes the stopping distance thirteen times as long
platforms: linux, windows
teaches: 22-03-units-and-limits-that-cannot-be-mixed
---

## Symptom

The robot overshoots every stop by metres and the margins in the planner look right.

## Cause

The limit came from a data sheet in km/h and was read as m/s. Stopping from 5 m/s took 8.331 m against 0.642 m.

## Fix

Make the unit part of the type and say it where the number is written.

```cpp
lim.max_v = Speed::from_kmh(5.0);
```
