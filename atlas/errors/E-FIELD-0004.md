id: E-FIELD-0004
title: Two transforms composed in the wrong order
match: a point moves by the wrong amount only once the robot turns
match: compose(shift, turn) is not compose(turn, shift)
match: a transform composed with its inverse is the identity, in the plane and in space
match: the order of composition matters, by a measurable amount
match: the chain map, odom, base, lidar carries a point to the map, and agrees with the plane arithmetic
platforms: linux, windows
teaches: 22-02-frames-for-a-field-robot
---

## Symptom

Everything is right while the robot drives straight and wrong the moment it turns.

## Cause

Transforms do not commute. A shift and a turn of 90 degrees applied in the two orders put a point 1.414 m apart.

## Fix

Compose right to left, check one chain against an independent calculation, and test with a turn.

```cpp
const Tf3 whole = compose(map_from_odom, compose(odom_from_base, base_from_lidar));
```
