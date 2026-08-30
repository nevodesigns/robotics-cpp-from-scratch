id: E-MATH-0005
title: A direction transformed as though it were a point
match: expected this to be true: near_vec.*direction
platforms: linux, windows
teaches: 06-04-transforms-and-frames
---

## Symptom

A velocity, a surface normal or a sensor bearing comes out wrong by a constant
amount. The error never changes size, never grows, and is exactly equal to some
mounting offset in the system.

## Cause

A direction was transformed with a point transform, so the translation was added
to something that has no position. A point is a place and moving the frame moves
it. A direction is an arrow with no location, and moving a frame does not change
which way north is.

The reason this survives so long is that the error is small, constant and
plausible. A sensor mounted five centimetres off the axis produces a bearing
wrong by five centimetres worth of offset, which looks like calibration error
rather than a bug.

## Fix

Keep the two operations separate and name them so the choice is deliberate:

```cpp
Vec3 apply_to_point(const Transform& t, const Vec3& p);       // rotate, then translate
Vec3 apply_to_direction(const Transform& t, const Vec3& d);   // rotate only
```

Where a library gives you a four by four matrix, the same distinction appears as
the fourth component: one for a point, zero for a direction. A zero there
multiplies the translation column away, which is exactly the rule above written
in matrix form.

Velocities, accelerations, angular rates, surface normals and bearings are all
directions. Positions and landmark locations are points.
