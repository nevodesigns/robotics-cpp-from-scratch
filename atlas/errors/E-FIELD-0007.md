id: E-FIELD-0007
title: A limit written in the wrong unit
match: a speed limit of 5 was meant in km/h and used as m/s
match: the stopping distance is many times longer than the plan says
match: a unit is part of the type, and the conversions are right
match: speed up slowly, brake harder, and the numbers are the ones the limits say
match: a limit read in the wrong unit makes the stopping distance thirteen times as long
match: the turn rate is eased as the speed is, and a change within the limit is not called limited
platforms: linux, windows
teaches: 22-03-units-and-limits-that-cannot-be-mixed
---

## Symptom

The robot overshoots every stop by metres, though the planner's margins look right. The limit came from a data sheet.

## Cause

A limit from a data sheet in km/h was read as m/s. Stopping from 5 m/s took 8.331 m, where 5 km/h stops in 0.642 m.

A number has no unit unless the code gives it one. 5 km/h is 1.39 m/s, and read as 5 m/s it is a speed more than three times too high, with a stopping distance more than twelve times too long.

## Fix

Make the unit part of the type, and say it where the number is written.

```cpp
rc::field::VehicleLimits lim;
lim.max_v = rc::field::Speed::from_kmh(5.0);
```
