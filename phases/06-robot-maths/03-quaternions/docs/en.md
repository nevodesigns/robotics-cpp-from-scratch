# Quaternions: Four Numbers, No Singularity, One Division to Repair

> You do not need to understand what a quaternion is to use one correctly. You need to know what it stores, how to combine two, and the one thing about it that will surprise you.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 06-02

## The Problem

The last lesson left two measurements.

Three angles collapse at one orientation: at a pitch of ninety degrees, adding
twenty degrees to both roll and yaw changed the rotation matrix by `1.11e-16`,
which is nothing at all.

Rotation matrices drift: a hundred thousand compositions left the columns off
unit length by `4.0e-12`, and repairing that means orthogonalising nine numbers.

Quaternions answer both. Four numbers, no orientation at which the
representation degenerates, and repair is one division. This lesson builds them,
and it does not require you to have any intuition for four dimensional algebra,
because none is needed to use them correctly.

## The Concept

### What it stores

A unit quaternion holds an axis and an angle, arranged so that composition is
cheap:

```
w = cos(angle / 2)
x = axis.x * sin(angle / 2)
y = axis.y * sin(angle / 2)
z = axis.z * sin(angle / 2)
```

That is the whole definition. The vector part points along the axis of rotation
and the scalar part records how far around, and everything else follows.

**The half angle is the surprise.** A quarter turn, ninety degrees, gives
`w = cos(45) = 0.707`, not `cos(90) = 0`. Every quaternion bug that is not a sign
error is somebody forgetting to halve the angle, and the symptom is a rotation of
exactly twice or half what was intended.

The axis must be a unit vector before it is scaled, which is where the zero
length guard from lesson 06-01 earns its place: an axis of no length has no
rotation to describe.

### Combining two is one multiplication

Applying one rotation after another is a quaternion product, and like matrices it
does not commute. Written with the scalar and vector parts separated, it is:

```
w = w1*w2 - v1 . v2
v = w1*v2 + w2*v1 + v1 x v2
```

Both products from lesson 06-01 appear, which is not a coincidence: the dot
product carries how much the rotations agree and the cross product carries the
turning between them.

Sixteen multiplications against the twenty seven a matrix product needs, on four
numbers rather than nine. For a robot composing transforms thousands of times a
second, that difference is real.

### Undoing one is negating three numbers

For a unit quaternion the inverse is the conjugate: keep `w`, negate `x`, `y` and
`z`. That is the same bargain the transpose gave for matrices, and it holds for
the same reason, that the thing has unit length.

### Rotating a vector

```
v' = q * (0, v) * conjugate(q)
```

Treat the vector as a quaternion with zero scalar part, multiply on both sides,
and take the vector part of the result. It is more work than a matrix multiply
for one vector, which is why code transforming a point cloud converts to a matrix
first and code storing an orientation keeps the quaternion.

### The one thing that will surprise you: q and minus q are the same rotation

Negate all four numbers and you have a different quaternion describing exactly
the same orientation. The representation covers each rotation twice.

This matters in three concrete places.

**Comparing orientations.** `q1 == q2` is wrong. Two quaternions describe the
same rotation when they are equal *or* when one is the negation of the other.

**Interpolating.** Blending between `q` and `-q` takes the long way round, all
the way about the sphere, when the two ends are the same orientation. Every
interpolation routine begins by checking the sign of the dot product and negating
one end if it is negative.

**Averaging or subtracting.** Neither is meaningful without resolving the sign
first.

The exercise tests this directly, because code that ignores it works perfectly
until an orientation happens to cross the boundary.

### Drift, and why repair is cheap

Quaternions drift too, and it is worth being precise about this because the
common claim is wrong.

Running the same measurement as the last lesson, a hundred thousand
compositions, on the same machine:

```text
rotation matrix   columns off unit length by   4.0e-12
quaternion        norm off one by              4.3e-12
```

They drift by the same amount. Quaternions are not more numerically stable in
that sense, and anyone who tells you they are has not measured it.

The difference is entirely in the repair. A matrix has to be orthogonalised:
three columns made unit length and mutually perpendicular, which is an iterative
or decompositional procedure and is only approximately right. A quaternion is
divided by its norm. One square root and four divisions, and the result is
exactly a unit quaternion.

That is the practical reason attitude estimators hold their state as a
quaternion and renormalise on every update: not because it drifts less, but
because putting it back costs almost nothing.

## Build It

Implement in `exercise/solution.hpp`:

- `from_axis_angle(axis, angle)`, remembering the half angle and normalising the
  axis, and answering the identity rotation for an axis of no length.
- `multiply(a, b)`, the product above.
- `conjugate(q)` and `norm(q)` and `normalized(q)`.
- `rotate(q, v)`, applying a rotation to a vector.
- `same_rotation(a, b, tolerance)`, true when the two describe the same
  orientation, which means allowing for the sign.

```
rcpp verify 06-03
```

## Use It

Eigen has `Quaterniond`, ROS uses quaternions in every pose message, and every
inertial measurement unit that reports orientation reports one. `slerp` is the
interpolation named above, and its first act is the sign check.

You will still convert to a matrix to transform many points, and to three angles
to show a human. Both are boundary operations. In between, orientation is stored
as four numbers, because those four have no orientation at which they misbehave.

## What Breaks First

- **Every rotation is twice or half what it should be.** The angle was not
  halved. See `E-MATH-0003`.
- **Orientations that are equal compare as different.** `q` and `-q` are the same
  rotation, so comparison has to allow for the sign. See `E-MATH-0003`.
- **The norm wandering from one.** Drift, repaired by dividing by the norm. See
  `E-MATH-0002`.
- **NaN from an axis of zero length.** Normalising nothing. See `E-NUM-0013`.

## Ship It

`Quat` joins `rc::math` and becomes the way every later phase stores an
orientation. Phase 13 composes them along a kinematic chain, phase 15 receives
them from an inertial sensor, and phase 16 keeps one as the attitude part of its
estimate, renormalising it on every update for the reason measured here.
