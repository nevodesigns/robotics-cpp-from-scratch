id: E-MATH-0005
title: A joint turned about an axis in the wrong frame
match: a joint turning about a different axis lifts the arm out of the plane
match: turning the second joint moves only what is beyond it
platforms: linux, windows
teaches: 13-01-where-the-tool-is
---

## Symptom

An arm that is correct for the first joint and wrong for every joint after it.
Bending the elbow swings the shoulder. A wrist rotation moves the tool in a
direction the wrist cannot move it.

Straightening the arm makes the error vanish, so it looks like a problem that
only appears in awkward configurations.

## Cause

The joint's rotation was applied before its fixed geometry:

```cpp
base_here = compose(base_here, joint_motion(joints[i], angle));   // the bug
base_here = compose(base_here, joints[i].parent_to_joint);
```

A joint's axis is written in **its own frame**, and its own frame does not exist
until the fixed transform has placed it. Turning first means turning about that
axis as expressed in the **parent's** frame, which is a different axis whenever
the fixed transform contains a rotation.

Which is why a straight arm hides it: with no rotation in the fixed geometry the
two frames are aligned, so the two axes coincide and both orders agree.

## Fix

Geometry first, then motion:

```cpp
base_here = compose(base_here, joints[i].parent_to_joint);
base_here = compose(base_here, joint_motion(joints[i], angle));
```

The general statement is worth keeping, because this shape appears well beyond
arms: **a vector is only meaningful together with the frame it is expressed in.**
An axis, a velocity, a force and an offset are all in some frame, and the bug is
always the same bug: using one in a frame it was not written for.

Two things that make it hard to write by accident.

Name the frame in the variable, as lesson 06-04 argues. `axis_in_joint` and
`axis_in_base` cannot be mixed up silently the way two things both called `axis`
can.

And test a joint whose fixed geometry contains a rotation, or the two orders
agree and the test proves nothing. In lesson 13-01 that is the joint given a
different axis from its neighbours, which lifts the arm out of the plane, and
it is the only test in the file that separates these two orderings.
