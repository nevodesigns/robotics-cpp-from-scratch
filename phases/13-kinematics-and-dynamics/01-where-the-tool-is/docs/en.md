# Where the Tool Is: Forward Kinematics as Composition

> Six joint angles go in and a position in space comes out. There is no new
> mathematics in this lesson: it is the transform from 06-04, applied twice per
> joint, in an order that is the whole difficulty.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 06-04, 03-03

## The Problem

An arm has six motors. Each reports an angle. Somewhere out at the end of it is
a gripper, and everything you might want to do with the arm starts with knowing
where that gripper is.

The temptation is trigonometry. For a two link arm in a plane it works and the
answer fits on one line. For six joints in three dimensions, with each joint's
axis pointing wherever the mechanical drawing says, it produces a page of sines
and cosines that is wrong in a way nobody can find.

The alternative is that you already have the tool. A joint is a frame that moves
relative to the frame before it, and lesson 06-04 built the type for exactly
that. Forward kinematics is composition, and this lesson is mostly about doing
it in the right order.

## The Concept

### A joint is two transforms, not one

Every joint contributes two separate things, and keeping them separate is what
makes the whole thing work:

- **Fixed geometry.** Where this joint sits relative to the previous one. It
  comes off the drawing and it never changes.
- **Motion.** A rotation about the joint's axis, by the current angle. This is
  the only part that varies.

```cpp
struct Joint {
  Transform parent_to_joint;   // fixed, from the drawing
  Vec3 axis;                   // unit vector, in this joint's frame
};
```

The tool position is every one of those composed, from the base outward.

### The order is the whole difficulty, and the naming rule is the answer

Composition is not commutative, and the result of getting it backwards is not
nonsense. It is a perfectly good transform between two frames, which happen to
be two frames nobody asked about. That is why it looks plausible.

Lesson 06-04's naming rule is what turns this from something to remember into
something to check:

```text
base_1 composed with 1_2   ->   base_2      the inner subscripts cancel
1_2   composed with base_1 ->   nothing with a name
```

Written that way the mistake is visible on the line rather than in the answer.

There is a second ordering inside each joint, and it is the one that catches
people. **Geometry first, then motion:**

```cpp
base_here = compose(base_here, joint.parent_to_joint);
base_here = compose(base_here, joint_motion(joint, angle));
```

The axis is written in the joint's **own** frame, and that frame does not exist
until the fixed transform has placed it. Turning first turns about that axis as
expressed in the parent's frame, which is a different axis whenever the fixed
geometry contains a rotation.

That last clause is why the bug survives. On a straight arm the two frames are
aligned, so both axes coincide and both orders agree.

### Check it against something that is not itself

Both ordering mistakes have the same property: they are **correct at the home
configuration**, because with every angle at zero the rotations are identity and
order stops mattering. The configuration everybody checks first is the one that
proves the least.

The way out is an answer derived without any of this machinery. A planar two
link arm has a closed form:

```text
x = l1 cos(q1) + l2 cos(q1 + q2)
y = l1 sin(q1) + l2 sin(q1 + q2)
```

Build that same arm out of joints and compare, across a grid rather than at a
handful of points. Measured, in the tests you are about to run:

```text
compared against the closed form at 3721 configurations,
worst disagreement 2.78e-16 metres
```

That is a check the machinery cannot pass by being consistently wrong. A round
trip through the chain and back would pass either way, which lesson 05-03 is
about.

### What does not need worrying about

Six joints is five quaternion compositions, and lesson 06-02 measured what
repeated composition does to a rotation **matrix**: it walks away from being a
rotation at all.

Measured here, a six joint chain leaves the quaternion with a norm of
`1.000000000000000`. Nothing needs renormalising. The drift 06-02 found comes
from repeatedly updating one rotation over thousands of steps, which is a
different situation from composing a handful of them once, and the difference is
worth knowing rather than guessing at.

### Angles you were not given

A caller who supplies four angles to a six joint arm has made a mistake, and
there are two ways to respond to it. Holding the missing joints at zero is
defensible and produces a pose. Reading past the end of the array is not an
answer at all, and it will usually produce a pose too.

Do the first, and let the caller **ask** whether the count matched, so that a
plausible answer to the wrong question can be noticed.

## Build It

Implement `Chain::frame_at` and `Chain::accepts` in `exercise/solution.hpp`.

```
rcpp verify 13-01
```

The suite checks the arm at configurations you can work out on paper, then
against the closed form across a grid, then with a joint whose axis differs from
its neighbours, which is the only test in the file that separates the two
orderings inside a joint.

## Use It

This is the shape for any serial mechanism: an arm, a pan and tilt head, a leg,
a camera on a gimbal on a vehicle. The frames chain the same way, and the same
naming rule keeps them straight.

It is also half of what a robot needs. This answers where the tool is given the
angles; the other direction, what angles put the tool somewhere, is the next
lesson and is a great deal harder.

## What Breaks First

- **A chain composed the wrong way round.** Plausible, and correct at the home
  configuration. See `E-MATH-0004`.
- **A joint turned before its own frame exists.** Correct for the first joint
  and wrong for every one after it. See `E-MATH-0005`.
- **Reading past the end of the angles.** See `E-CPP-0007`.

## Ship It

`Chain` joins the new `rc::kin` module, which is what the rest of this phase is
built on. It is also the first thing in the curriculum that needed the
`Transform` from phase 06 to exist, rather than describing it.
