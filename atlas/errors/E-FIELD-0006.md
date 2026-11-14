id: E-FIELD-0006
title: A missing frame answered with the identity
match: a lookup for a frame that was never added puts the robot at the origin
match: between returns false for an unknown frame, two trees or a loop
match: an unknown frame, two separate trees and a loop all give no transform instead of a wrong one
platforms: linux, windows
teaches: 22-02-frames-for-a-field-robot
---

## Symptom

One sensor's points land on top of the robot, and nothing says why.

## Cause

A lookup that cannot find a frame returned the identity transform, which is a valid answer.

## Fix

Return a failure for an unknown frame, for frames in different trees and for a loop, and make the caller handle it.

```cpp
if (!tree.between("lidar", "map", out)) return;
```
