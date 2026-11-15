id: E-FIELD-0006
title: A missing frame answered with the identity
match: a lookup for a frame that was never added puts the robot at the origin
match: between returns false for an unknown frame, two trees or a loop
match: an unknown frame, two separate trees and a loop all give no transform instead of a wrong one
platforms: linux, windows
teaches: 22-02-frames-for-a-field-robot
---

## Symptom

One sensor's points land on top of the robot, and nothing says why. A frame was misspelt or not yet published.

## Cause

A lookup that could not find a frame returned the identity transform, which looks like a valid answer.

The identity transform is a valid transform: it says two frames are the same. Returning it for a frame that is not there gives the caller points in the right format and the wrong place.

## Fix

Return a failure for an unknown frame, for frames in different trees and for a loop, and make the caller handle it.

```cpp
rc::field::Tf3 out;
if (!tree.between("lidar", "map", out)) return;   // no transform, no points
```
