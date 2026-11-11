# How Wrong a Pose Is: Position Error, Orientation Error as a Rotation Vector, and Why Euler Angles Cannot Be Subtracted

> Say how far one orientation is from another with the turn that takes one to the other, and be careful at both ends of the angle.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 13-12, 06-03

## The Problem

A controller steers a tool to a pose and needs a number for how far it has to go. For position it is a subtraction. For orientation, subtracting the Euler angles gives answers that are a full turn out, and are not the angle between the two orientations even when they are not. The measure that works is the turn from one to the other, and it has two ends at which the obvious formula fails.

## The Concept

### The turn between two orientations

The rotation matrix of the target times the transpose of the current one is the turn that takes the current to the target. Written as a vector, the axis times the angle, it is the **rotation vector**: zero when the orientations are the same, small when they are close, and the same however they were described. `rotation_vector` recovers the axis times the angle for **8** angles from 0 to 3.1415 about the axis (1, 2, 2) / 3, **0** worse than 1e-6.

### Euler angles cannot be subtracted

A yaw of -1 degree against a yaw of 359 degrees: the difference of the Euler angles is **-360** degrees, and the rotation error is **9.4e-16** rad. They are the same orientation. A real difference of 10 degrees is 10 degrees however the angles are written, about z and in the positive direction, and the other way round it is the other sign. See `E-KIN-0032`.

### A very small turn

The usual formula takes the arc cosine of (trace - 1) / 2. For a turn of 1e-10 about z that number rounds to 1, and the angle is **0**: the answer is gone. The antisymmetric part of the matrix is 2 sin(angle) times the axis, which for a small angle is 2 times the angle, and it holds the answer: **1e-10**. A controller that uses the arc cosine has a dead zone around the target and never quite converges. See `E-KIN-0033`.

### A half turn

At a half turn the sine of the angle is zero, and the antisymmetric part, divided by it, is noise divided by noise. The axis is in the symmetric part: R is 2 k kT less I. A half turn about x was **pi** along x. Nearly a half turn, pi less 1e-6, about +y was y = **3.14159** and about -y it was **-3.14159**: the sign comes from the antisymmetric part, small as it is. See `E-KIN-0034`.

### Close enough in both

A pose in the right place and a tenth of a radian out: position error **0** m, rotation error **0.1** rad, not converged to a millimetre and a milliradian. A pose a centimetre out in x and right in orientation was not converged either. `converged` needs both.

## Build It

`rotation_vector`, `pose_error` and `converged` in `exercise/solution.hpp`.

```
rcpp verify 13-13
```

## Use It

`rc/kin/pose_error.hpp` joins its module.

```cpp
const rc::kin::PoseError e = rc::kin::pose_error(current_tool, target_tool);
if (rc::kin::converged(e, 0.001, 0.002)) done();
else steer(e.position, e.rotation);               // a velocity of the tool is proportional to the error vector
```

**What this does not do.** It says how far, in the base frame, and not which way to turn each joint: that is the Jacobian (lesson 13-03 for a planar arm). The error is the turn the short way round, in [0, pi], so a target a little over a half turn away is reached the other way, which is right for a tool and wrong for a cable that must not wind up. It treats a metre and a radian as comparable only through the two tolerances: a single number for both needs a choice of scale. And the cutoffs, 1e-7 and 1e-4 from a half turn, are for double precision, and a float would want wider ones.

## What Breaks First

- **Euler angles subtracted.** See `E-KIN-0032`.
- **The arc cosine at a small angle.** See `E-KIN-0033`.
- **The axis of a half turn, and a pose that is right in one way.** See `E-KIN-0034`.

## Ship It

`rc/kin/pose_error.hpp` holds `rotation_vector`, `PoseError`, `pose_error` and `converged`.

The lesson after next uses it to stop a numerical solver, and the capstone to say how close the arm got.
