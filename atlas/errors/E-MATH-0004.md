id: E-MATH-0004
title: A frame composed the wrong way, or an inverse that only negates
match: expected this to be true: near_vec
match: expected .*translation.* to be
platforms: linux, windows
teaches: 06-04-transforms-and-frames
---

## Symptom

An arm reaches confidently to entirely the wrong place. A camera detection lands
somewhere plausible but wrong. Nothing crashes, nothing warns, and the number
looks like a position.

Or an inverse works perfectly in testing and fails the moment something rotates.

## Cause

Two shapes, both common enough to name together.

The composition ran in the wrong direction, or two transforms were chained whose
frames do not meet. Composing T_world_base with T_gripper_camera is meaningless,
but nothing in the type system objects.

Or the inverse negated the translation without rotating it. The original
translation is expressed in the original frame, and the inverse needs it in the
rotated one. The negation only version is exactly correct whenever the rotation
is the identity, so it passes any test written with pure translations and waits
for the first real rotation.

## Fix

Name every transform for its two frames, destination first, and check that
adjacent names cancel:

```
T_world_base * T_base_gripper = T_world_gripper
```

That turns a geometry question into a spelling check.

For the inverse, rotate the negated translation by the undone rotation:

```cpp
const Quat undo = conjugate(t.rotation);
return Transform{undo, rotate(undo, scale(t.translation, -1.0))};
```

Test it with a transform that actually rotates. A suite that only translates
cannot tell the two implementations apart.
