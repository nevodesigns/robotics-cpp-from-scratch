id: E-MATH-0004
title: A chain of transforms composed the wrong way round
match: the chain agrees with the closed form
match: turning the second joint moves only what is beyond it
platforms: linux, windows
teaches: 13-01-where-the-tool-is
---

## Symptom

An arm whose tool ends up somewhere reasonable and wrong. It moves when the
joints move, the distances are about right, and the position does not match
where the machine actually is.

The tell is that the **home configuration is usually correct**. With every angle
at zero the rotations are all identity, so the order does not matter and the
answer comes out right, which is exactly the configuration everybody checks
first.

## Cause

The composition runs the wrong direction:

```cpp
base_here = compose(joint.parent_to_joint, base_here);   // the bug
```

Composition is not commutative, and the result of the wrong order is not
nonsense. It is a perfectly good transform between two frames, which happen to
be two frames nobody asked about.

## Fix

Use the naming rule from lesson 06-04 and let it do the checking. Name every
transform for the two frames it relates, destination first:

```text
base_1 composed with 1_2   ->   base_2       the inner subscripts cancel
1_2   composed with base_1 ->   nothing that has a name
```

```cpp
base_here = compose(base_here, joints[i].parent_to_joint);
base_here = compose(base_here, joint_motion(joints[i], angle));
```

Written that way the mistake is visible on the line rather than in the result,
because the adjacent subscripts do not match.

**Test it against something derived independently.** A planar two link arm has a
closed form:

```text
x = l1 cos(q1) + l2 cos(q1 + q2)
y = l1 sin(q1) + l2 sin(q1 + q2)
```

Compare the chain against that across a grid of configurations rather than at a
handful of points. Measured in lesson 13-01: 3721 configurations, worst
disagreement 2.8e-16 metres. That is a check the machinery cannot pass by being
consistently wrong, which a round trip through itself would.

And avoid checking only the home configuration, for the reason above: it is the
one configuration where this bug does not show.
