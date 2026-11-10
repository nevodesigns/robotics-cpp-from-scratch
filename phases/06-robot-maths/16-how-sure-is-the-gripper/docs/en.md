# How Sure Is the Gripper: Carrying the Doubt in the Joints Through the Arm to the Tool

> The joints are known to a few hundredths of a radian. How well is the tool known? The answer depends on where
> the arm is, and a formula gives it, and a few thousand random trials check it.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 06-04, 06-05, 06-11

## The Problem

An encoder says the shoulder is at 0.40 rad, give or take 0.02, and the elbow at 1.00, give or take 0.02. Where is the tool, and how sure is the robot of it?

Phase 06's last lessons built the pieces: frames and transforms (06-04) say where the tool is, the derivative (06-05) says how it moves, and the covariance (06-11) says what a doubt looks like. This lesson joins them: the doubt in the joints, carried through the arm to the tool. It is the question in the phase's ending: where the gripper is, and how you know.

## The Concept

### Where the tool is

For a planar two link arm, `x = l1 cos q1 + l2 cos(q1 + q2)` and `y = l1 sin q1 + l2 sin(q1 + q2)`. With links of 0.5 and 0.4 m, straight out the tool is at (0.9, 0), straight up at (0, 0.9), and with the elbow at 90 degrees at (0.5, 0.4).

### How a small joint move moves the tool

The **Jacobian** is the four numbers: the change in x and y for a small change in each joint. Written out, with `s1 = sin q1`, `c12 = cos(q1 + q2)` and so on:

```
dx/dq1 = -l1 s1 - l2 s12      dx/dq2 = -l2 s12
dy/dq1 =  l1 c1 + l2 c12      dy/dq2 =  l2 c12
```

Against a central difference at three poses, the worst difference was **5.7e-11**.

### The doubt at the tool

If the joints are off by a small `dq`, the tool is off by `J dq`. The covariance of `J dq` is `J C J'`, where `C` is the covariance of the joints. For links of 1 and the elbow at 90 degrees, `J = [[-1, -1], [1, 0]]`. With joint spreads of 0.2 and 0.3 rad the tool's variances were **0.13** in x and **0.04** in y, with a covariance of **-0.04**: x is `-q1 - q2`, y is `q1`. See `E-MATH-0034`.

### Checking by trying it

`sampled_tool_covariance` draws the joint angles at random, works out where the tool really is each time, and takes the covariance of the positions. It needs no approximation, only patience. At a shoulder of 0.4 and an elbow of 1.0 rad, 20000 trials:

| joint spread | variance of x, formula | sampled |
|---|---|---|
| 0.01 rad | 5.02e-05 | 5.03e-05 |
| 0.05 rad | 1.26e-03 | 1.25e-03 |
| 0.3 rad | 4.52e-02 | 4.00e-02 |

For small errors the formula agrees. For large errors the arm's sines and cosines show, and the linear estimate is out by more than a tenth. See `E-MATH-0035`.

The 95 percent ellipse of the propagated doubt held **95.1** percent of 20000 trials, for joint errors of 0.02 rad.

### The shape depends on the pose

Joint errors of 0.02 rad each, the shoulder at 0.4. The spread of the tool along and across the long axis of its ellipse:

| elbow | along (mm) | across (mm) | ratio |
|---|---|---|---|
| 1.57 | 13.97 | 5.73 | 2.4 |
| 0.50 | 19.09 | 2.01 | 9.5 |
| 0.05 | 19.69 | 0.20 | 97.0 |

Near a straight arm the Jacobian loses a direction, the singularity of lesson 13-04, and the doubt becomes a thin line. A single "tool accuracy" number hides that. See `E-MATH-0036`.

## Build It

`tool_position`, `jacobian` and `propagate` in `exercise/solution.hpp`. The types and `sampled_tool_covariance` are given, along with the covariance functions of lesson 06-11.

```
rcpp verify 06-16
```

## Use It

`rc/math/propagate.hpp` joins `rc::math`. Its Jacobian type there is `ToolJacobian` and the function `tool_jacobian`.

```cpp
const Cov2 tool = propagate(tool_jacobian(l1, l2, q1, q2), joint_covariance);
const Axes axes = principal_axes(tool);
```

**What this does not do.** It is a planar arm with two joints. A real arm has six and a Jacobian that is six by six, and the same formula, `J C J'`, applies with larger matrices. It carries the doubt in the joint angles and none from the link lengths, the base position or the calibration, which can dominate: lesson 16-09 measured a related case, wheel size and wheel base, for a rover. It assumes the joint errors are independent unless a covariance says otherwise, and encoder errors seldom move together but a shared gear play might. And it is first order: for large errors use the trials.

## What Breaks First

- **Joint errors taken for tool errors.** The pose is ignored. See `E-MATH-0034`.
- **A linear estimate for large errors.** It is out by a tenth or more. See `E-MATH-0035`.
- **One number for the tool's accuracy.** It hides a direction. See `E-MATH-0036`.

## Ship It

`rc/math/propagate.hpp` holds `tool_position`, `ToolJacobian`, `tool_jacobian`, `propagate` and `sampled_tool_covariance`.

That is phase 06's ending: where the gripper is, in the world frame, and how you know, to 0.2 mm in one direction and 20 mm in another, at the same joint accuracy, and which of the two the task needs.
