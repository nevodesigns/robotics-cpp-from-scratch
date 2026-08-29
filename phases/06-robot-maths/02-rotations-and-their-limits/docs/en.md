# Rotations: Matrices, Order, and the Angle That Vanishes

> Three angles describe an orientation right up until they do not, and the moment they stop is the moment a robot arm is pointing straight up.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 06-01

## The Problem

An arm needs to know where its gripper is pointing. A drone needs to know which
way is down. A camera needs to turn what it sees into where that thing is.

All three are the same question: given an orientation, where does a direction end
up. The obvious answer is three angles, roll, pitch and yaw, because that is how
people describe orientation out loud.

Three angles work, and then at one particular orientation two of them become the
same angle and the description silently loses a degree of freedom. It is called
gimbal lock, it happens when a robot is pointing straight up or straight down,
and straight up is not an unusual place for an arm to point.

This lesson builds rotations properly, measures the failure, and earns the next
lesson rather than asserting it.

## The Concept

### A rotation is a matrix that keeps lengths

In two dimensions, turning a vector by an angle is:

```
x' = x cos(a) - y sin(a)
y' = x sin(a) + y cos(a)
```

which is a matrix multiplying a vector. The columns of that matrix are where the
x axis and the y axis end up, and that is the useful way to read any rotation
matrix: **each column is where one axis went.**

What makes it a rotation rather than any other matrix is a property called being
orthonormal: every column has length one, and every pair of columns is
perpendicular. That is exactly the statement that the transformation keeps
lengths and angles unchanged, which is what turning something means.

The property is worth testing for directly, because it is the thing that breaks
in the failure mode below.

### Composing rotations is multiplying, and order matters

Applying one rotation then another is the product of their matrices. The
surprise, and it is a genuine surprise, is that the order changes the answer:

```
Rx(90) then Ry(90)   is not   Ry(90) then Rx(90)
```

Try it with a book. Rotate it ninety degrees about the axis pointing at you, then
ninety about the vertical, and note where the cover faces. Start again and do the
two in the other order. The book ends up somewhere else.

Rotation is not commutative, and every convention for combining angles therefore
has to state its order. When a library says roll pitch yaw, it means a specific
product in a specific sequence, and using a different one silently gives wrong
answers everywhere. This lesson uses:

```
R = Rz(yaw) * Ry(pitch) * Rx(roll)
```

which is the common aerospace convention and the one ROS uses.

### The transpose is the inverse

For a rotation matrix, undoing it is transposing it. No inversion algorithm and
no division, because the columns being orthonormal is exactly the condition that
makes the transpose an inverse.

That is worth knowing beyond the saving: if transposing a matrix does not undo
it, the matrix is not a rotation any more, which is the next problem.

### Gimbal lock, measured

Set pitch to ninety degrees, pointing straight up. Now the roll axis and the yaw
axis have been turned onto each other, so rolling and yawing do the same thing.
Two of the three numbers have become one.

The consequence is concrete and the exercise measures it. With pitch at ninety
degrees, `from_rpy(roll, pitch, yaw)` and `from_rpy(roll + d, pitch, yaw + d)`
produce the same matrix for any `d`. On the machine this lesson was written on,
with `d` of 0.37 radians, a little over twenty degrees added to both:

```text
largest change in any matrix entry:  1.11e-16
```

That is zero to the last bit a double can hold. Two descriptions differing by
twenty degrees in two of their three numbers, and the orientation is identical.
A controller trying to hold an attitude near there watches the description swing
wildly while the machine barely moves.

It is not a bug in the arithmetic. Three numbers simply cannot cover all
orientations smoothly, and no choice of axis order avoids it, only moves where it
happens. That is why the next lesson exists.

### Drift

Multiply rotations together thousands of times, as any simulation or dead
reckoning loop does, and rounding accumulates. The columns slowly stop being unit
length and stop being perpendicular, and the matrix stops being a rotation. It
starts scaling and shearing what it is meant only to turn.

The exercise measures that too. After a hundred thousand compositions, which a
loop at a kilohertz reaches in under two minutes:

```text
columns off unit length by:  4.0e-12
```

Small, and growing, and one directional: it never comes back on its own. The fix
is renormalisation, periodically forcing the columns back to orthonormal, and
anything long running has to do it.

## Build It

Implement in `exercise/solution.hpp`:

- `rotation_x`, `rotation_y`, `rotation_z`, each for one axis.
- `multiply(a, b)` and `apply(m, v)`.
- `transposed(m)`.
- `is_orthonormal(m, tolerance)`, checking every column has length one and every
  pair is perpendicular.
- `from_rpy(roll, pitch, yaw)`, using `Rz * Ry * Rx` as stated above.

```
rcpp verify 06-02
```

The tests include the gimbal lock demonstration and a drift measurement over a
hundred thousand compositions.

## Use It

Eigen has all of this as `Matrix3d` and `AngleAxisd`, and its `.transpose()` is
the same trick for the same reason.

Rotation matrices remain the right representation when you are transforming many
points at once, because applying one is nine multiplications and it composes with
translation into the single matrix of the next lesson in this phase. They are the
wrong representation for storing or interpolating an orientation, which is what
quaternions are for.

## What Breaks First

- **Two orientations that should differ come out identical.** Gimbal lock, and
  the reason is structural rather than a mistake in your code. See
  `E-MATH-0001`.
- **A matrix that no longer keeps lengths.** Drift from repeated multiplication.
  Renormalise. See `E-MATH-0002`.
- **Everything rotates the wrong way.** A sign in the matrix, or the composition
  applied in the opposite order. See `E-NUM-0004`.

## Ship It

`Mat3` and these operations join `rc::math`. The measurement in this lesson is
the artifact that matters most: you now have a number showing why an orientation
should not be stored as three angles, which is the argument the next lesson
builds on.
