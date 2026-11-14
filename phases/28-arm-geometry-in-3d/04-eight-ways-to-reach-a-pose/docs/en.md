# Eight Ways to Reach a Pose: Closed Form Inverse Kinematics for an Arm With a Spherical Wrist, and the Arms It Does Not Work For

> If the last three axes meet, the arm can be solved with a pencil, and the answer is eight.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 28-03, 13-02

## The Problem

Forward kinematics (28-02) is multiplication. The question that a program for an arm is asked far more often goes the other way: here is where the tool is to be; what do the joints have to be? That is inverse kinematics, and for most arms it has no tidy answer. For a family of arms it has: those whose last three axes meet in a point, a spherical wrist. Most industrial six joint arms were designed that way, for exactly this reason.

## The Concept

### Split the arm in two

If the last three axes meet, the point where they meet, the wrist centre, does not move when the wrist turns. It is fixed by the first three joints alone, and it is where the flange is less the last link along the tool's axis (the flange less `d6` along its z axis). So:

1. **Position, from the first three joints.** The wrist centre is at a place. The first joint turns the arm toward it: `atan2(y, x)`, or a half turn round, with the reach negative (the arm reaching over its shoulder). That leaves a plane: a shoulder and an elbow reaching a point, which is a triangle of upper arm, forearm and the line to the point. The law of cosines gives the elbow, and there are two elbows, up and down; then the shoulder follows from the geometry.
2. **Orientation, from the last three.** The first three joints have left a rotation to be done by the wrist: `R36 = R03 transposed times R`. For this wrist that is a Z Y Z Euler angle, with two solutions, and with a singularity: where the middle joint is zero the first and last wrist joints turn about the same axis and only their sum is fixed.

Two shoulders, two elbows and two wrists make eight. For 40 reachable poses the solver gave 8.0 solutions on average, the joint angles that made the pose were among them in 40 of 40, and every one of the solutions, put back through forward kinematics, reached the pose to 0.0000000000 m and 0.0000000000 in rotation. That is the test that matters: not "did it give the angles I started with" but "does every answer reach the pose". See `E-ARM-0010`.

### When there are fewer

A pose 1.5 m from the base gave 0 solutions (the arm cannot reach it) and the solver says so, with an empty list: a solver that returns something for an unreachable pose is returning a lie. At the very edge of the reach, the elbow straight, up and down are the same: 4 solutions. With the wrist straight (joint 5 at zero) the arm configuration that made the pose has one wrist solution and not two, so there were 6 solutions and not 8, and in that configuration the angles of joints 4 and 6 separately mean nothing, only their sum. See `E-ARM-0011`.

### Which of the eight

The eight are all right and a robot can take only some of them. With limits of plus and minus 120 degrees on every joint, 1 of the 8 were allowed for a typical pose. Picking among those is a choice (the nearest to where the arm is now, so that it does not swing round; away from a singularity; clear of an obstacle) and belongs to the caller. See `E-ARM-0012`.

### Not every arm

The closed form needs the last three axes to meet. The UR5 does not have a spherical wrist: its fifth joint is offset by 0.09465 m (the table's `d5`), and `has_spherical_wrist` said 0. The Panda has seven joints and an offset elbow. For such arms a closed form is hard or impossible and the next lesson solves them by iteration.

## Build It

`ik_spherical` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- The first joint is atan2 of the wrist centre (or that plus a half turn, with the radius negative); the elbow comes from sin(theta3) = (X^2 + Y^2 - a2^2 - d4^2) / (2 a2 d4), two angles; theta2 = atan2(Y, X) - atan2(-d4 cos(theta3), a2 + d4 sin(theta3)); the wrist is a Z Y Z Euler angle of R03 transposed times the flange's rotation, with its two solutions and its singular case.

As shipped, 3 of the 4 tests fail, and these are the ones your code has to turn green:

- For many poses the angles that made the pose are among the solutions and every solution reaches it.
- A pose out of reach has no solution, at the very edge the elbow up and down are one, and a straight wrist has fewer.
- The limits of the joints take some of the eight away.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 28-04
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/arm/ik_closed.hpp`.

```cpp
const auto all = rc::arm::ik_spherical(table, target, base, tool);
const auto allowed = rc::arm::within_limits_only(table, all);
// pick the one nearest to the present joint angles
```

**What this does not do.** It is for this kind of arm: an elbow with its first joint vertical, a shoulder offset of zero, a spherical wrist; a different layout of axes (an offset at the shoulder, a different twist) is a different derivation. It gives all the solutions and picks none. The wrist's singular case returns one solution with `theta4` set to zero, which a controller that is moving through the singularity will not like. It does not look at collisions or at the joint speeds needed to get there. The dimensions are typical, not a product's.

## What Breaks First

- **A solver that was only tested on its own answers.** See `E-ARM-0010`.
- **A solver that returns something for an unreachable pose.** See `E-ARM-0011`.
- **A solution that is right and not allowed.** See `E-ARM-0012`.

## Ship It

`rc/arm/ik_closed.hpp` holds `elbow_arm`, `has_spherical_wrist`, `wrap_angle`, `ik_spherical` and `within_limits_only`.
