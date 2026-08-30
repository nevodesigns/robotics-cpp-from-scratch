# Transforms: Where Is the Gripper, in Which Frame, and How Do You Know

> Almost every serious robotics bug is a frame bug. The fix is not cleverness, it is a naming rule you follow without exception.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 06-03

## The Problem

A camera on a robot's wrist sees a cup. The camera reports the cup is 30
centimetres ahead and 5 to the left.

Ahead of what? The camera. The arm cannot use that. The arm needs the cup's
position relative to its base, which requires knowing where the wrist is
relative to the base, which requires the shoulder and elbow angles, and every one
of those relationships is a rotation and a translation.

Get one of them backwards and the arm reaches confidently to entirely the wrong
place. It will not crash, nothing will warn, and the number will look plausible.

This is the most common serious bug in robotics, and it has a discipline rather
than a trick.

## The Concept

### A transform is a rotation and a translation

```cpp
struct Transform {
  Quat rotation;
  Vec3 translation;
};
```

It describes where one frame sits relative to another: turn by the rotation, then
move by the translation.

### Points move, directions only turn

This distinction catches people and it matters.

A **point** is a place. Transforming it applies the rotation and then adds the
translation, because moving the frame moves the place.

A **direction** is an arrow with no location: a velocity, a surface normal, the
way a sensor is pointing. Transforming it applies **only** the rotation. Adding
the translation would be meaningless, because moving a frame does not change
which way north is.

```cpp
apply_to_point(t, p)      // rotate, then translate
apply_to_direction(t, d)  // rotate only
```

Using the wrong one gives an answer wrong by exactly the translation, which for a
sensor a few centimetres off the axis is a small, plausible, persistent error of
the kind that survives review for months.

### The naming rule, which is the whole lesson

Name every transform for the two frames it relates, destination first:

```
T_world_base      where the base is, expressed in the world frame
T_base_gripper    where the gripper is, expressed in the base frame
```

Now composition has a property that makes mistakes visible. **Adjacent subscripts
must match, and they cancel:**

```
T_world_base * T_base_gripper  =  T_world_gripper
         ^^^^    ^^^^
```

If the inner names do not match, the composition is wrong, and you can see it
without reasoning about geometry at all. It becomes a spelling check.

```
T_world_base * T_gripper_camera     // base against gripper: wrong, and visible
```

Every robotics team that survives adopts some version of this. ROS calls the
relationships a transform tree and gives you `tf2` to store and look them up, and
`tf2` is essentially bookkeeping for exactly this rule.

### Inverting is not negating the translation

Here is the mistake that looks right:

```cpp
Transform inverse(const Transform& t) {
  return Transform{conjugate(t.rotation), scale(t.translation, -1.0)};   // wrong
}
```

The rotation part is right. The translation is not, because the original
translation is expressed in the *original* frame, and the inverse needs it
expressed in the *rotated* one.

The correct form rotates the negated translation by the inverted rotation:

```cpp
const Quat r = conjugate(t.rotation);
return Transform{r, rotate(r, scale(t.translation, -1.0))};
```

The wrong version is exactly right whenever the rotation is the identity, which
is why it survives testing: somebody writes a test with no rotation, it passes,
and the bug waits for the first transform that actually turns.

The exercise tests both cases deliberately for that reason.

### Composition order

`compose(a_b, b_c)` gives `a_c`, and the rotations multiply in the same order
quaternions do. The translation is the outer translation plus the outer rotation
applied to the inner translation, because the inner offset has to be expressed in
the outer frame before the two can be added.

## Build It

Implement in `exercise/solution.hpp`, using `rc::math` from the previous
lessons:

- `apply_to_point(t, p)` and `apply_to_direction(t, d)`.
- `compose(a_b, b_c)`, giving `a_c`.
- `inverse(t)`, correctly, including the rotation of the translation.

```
rcpp verify 06-04
```

The tests build a three link arm, world to base to shoulder to gripper, and ask
where the gripper is in the world frame. That is the question phase 13 is
entirely about.

## Use It

Production code stores this as a four by four matrix, where composition is one
matrix multiply and the bottom row is always `0 0 0 1`. Eigen calls it
`Isometry3d`. The four by four form is faster for transforming many points and
stores the same information.

ROS gives you `tf2`, which holds a tree of these relationships with timestamps
and answers "where is A relative to B" by walking the tree. The timestamps matter
more than they look: asking where the gripper was is a different question from
where it is, and a transform tree that ignores time answers the wrong one during
motion.

## What Breaks First

- **An arm that reaches to the wrong place, confidently.** Two frames composed in
  the wrong order, or a transform used in the wrong direction. Check the adjacent
  subscripts. See `E-MATH-0004`.
- **An error exactly equal to some fixed offset.** A direction was transformed as
  a point, so a translation was added to something that has no position. See
  `E-MATH-0005`.
- **An inverse that works until something rotates.** The translation was negated
  without being rotated. See `E-MATH-0004`.

## Ship It

`Transform` joins `rc::math` and is what the whole of phase 13 is built on. The
naming rule goes with it and matters more than the code: name every transform
for its two frames, destination first, and let the subscripts do the checking.
