id: E-FIELD-0016
title: A lost lidar beam read as a range
match: a beam that returned nothing is stored as zero or the maximum range and becomes an obstacle
match: the lidar reports infinity for a beam that saw nothing
platforms: linux, windows
teaches: 22-06-sensors-that-fail-like-the-field
---

## Symptom

A phantom wall appears in front of the robot for a moment, or a gap is filled in with something that is not there.

## Cause

About 0.90 percent of the beams that should have returned did not, and the code turned them into a number.

## Fix

Return infinity for a beam that saw nothing and handle it explicitly everywhere ranges are used.

```cpp
if (r == no_return()) continue;
```
