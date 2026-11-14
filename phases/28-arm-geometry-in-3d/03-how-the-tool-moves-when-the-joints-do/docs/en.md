# How the Tool Moves When the Joints Do: The Jacobian of an Arm in Three Dimensions, Checked Against Finite Differences, and Torque From Force

> The Jacobian is the arm's speed gearbox, and the same table gives the force gearbox for free.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 28-02, 13-08

## The Problem

Forward kinematics (28-02) gives a pose for a set of joint angles. Two questions need how the pose changes as the joints move: how fast will the tool go if the joints turn at these rates, and which way must the joints turn to take the tool somewhere? And a third, which turns out to be the same question: what torques does the arm need to push on the world with the tool? All three are answered by one matrix, the Jacobian.

## The Concept

### One column for each joint

A revolute joint turning at a rate `w` gives the tool an angular velocity of `w` about the joint's axis `z` and, because the tool is some way from the axis, a linear velocity of `w` times `z cross (p - o)`, where `p` is the tool's place and `o` is any point on the axis. The column of that joint is those two vectors, and the Jacobian is the six rows by `n` columns that you get from all the joints. Then:

```
tool velocity (3 linear, 3 angular) = J  *  joint rates
```

Everything depends on where the axis is, and this is where the convention of 28-01 comes back: in the standard convention the axis of joint `i` is the z axis of the frame before its row; in the modified convention it is the z axis of the frame after it. Built with the other convention's rule, the Panda's Jacobian was 1.29 out; with its own rule, it matched finite differences to 0.000000000144 (the UR5's to 0.000000000144) over 40 poses. See `E-ARM-0007`.

### Checked against differences

The numerical Jacobian differences the forward kinematics (move each joint a hair either way: the tool's place changes by the linear column; the small turn between the two poses gives the angular column). It is slow and noisy and exactly the right thing to check the fast and exact one against, and to keep as a test: the Jacobian is the one piece of an arm's code that a sign error makes subtly wrong and that nothing else will tell you.

### A tool offset moves the linear part

A tool offset of 0.15 m moves the point at which the tool is measured, so it changes the linear part (the lever arm is longer) and leaves the angular part alone (every point of a rigid tool turns together). A Jacobian worked out for the flange and used for the gripper tip is wrong by exactly that. See `E-ARM-0008`.

### Force by the same matrix, transposed

The joints and the tool do the same work: the power of the tool, wrench times velocity, equals the power of the joints, torque times rate. So the torques that hold a wrench at the tool are `tau = J transpose * wrench`. The two powers agreed to 0.000000000000000 W here. And it shows in the particulars: a vertical force of 10 N asked 0.000 N m of the first joint, whose axis is vertical (it cannot lift anything), and a great deal of the second. This is the force gearbox of 13-08 in three dimensions, and the start of gravity compensation (phase 29). See `E-ARM-0009`.

## Build It

`jacobian` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- The linear part of column i is the axis z cross (the tool point minus the point on the axis); the angular part is the axis z.

As shipped, 5 of the 6 tests fail, and these are the ones your code has to turn green:

- The analytic Jacobian is the finite difference one, for both arms, in both conventions' own rule.
- The rule of the other convention gives a Jacobian that is wrong.
- Joint rates give the tool the velocity the Jacobian says.
- A tool offset changes the linear part of the Jacobian and not the angular part.
- A vertical force at the tool asks nothing of the first joint, whose axis is vertical.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 28-03
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/arm/jacobian.hpp`.

```cpp
const rc::arm::Jacobian j = rc::arm::jacobian(table, q, base, tool, table.convention);
double v[6];
rc::arm::tool_twist(j, qdot, v);
const auto tau = rc::arm::joint_torques(j, wrench);
```

**What this does not do.** The Jacobian is the geometric one, with angular velocity in the base frame; the one that takes the time derivative of roll, pitch and yaw (the analytic one) is different and has the hole of 28-02. Rows are in the base frame: a wrench or a velocity in the tool frame must be rotated first. It is for revolute joints and at most seven of them. The force result is static: torques to hold a wrench against a standing arm, not the ones to accelerate it (phase 29). It says nothing about the friction in the joints, so the torque a motor gives and the one at the tool are not the same.

## What Breaks First

- **A Jacobian read with the other convention's rule.** See `E-ARM-0007`.
- **A Jacobian for the flange used for the tip.** See `E-ARM-0008`.
- **A torque the wrong way round.** See `E-ARM-0009`.

## Ship It

`rc/arm/jacobian.hpp` holds `Axis`, `joint_axes`, `Jacobian`, `cross3`, `jacobian`, `tool_twist`, `joint_torques`, `numeric_jacobian` and `jacobian_gap`.
