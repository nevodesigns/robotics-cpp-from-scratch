id: E-FIELD-0040
title: A wall laid along the x axis whatever its direction
match: only the walls parallel to an axis are in the right place
match: a box for a wall is not turned by the angle of the segment
platforms: linux, windows
teaches: 22-13-a-world-for-gazebo
---

## Symptom

The room is right where the walls are axis aligned and wrong everywhere else.

## Cause

The box was given the length and no angle. A wall from (0,0) to (3,4) is 5.000 m long turned 0.927295 rad.

## Fix

Turn the box by atan2(dy, dx) and put its centre at the middle of the two ends.

```cpp
b.yaw = std::atan2(w.y1 - w.y0, w.x1 - w.x0);
```
