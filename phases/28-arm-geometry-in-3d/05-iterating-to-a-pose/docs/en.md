# Iterating to a Pose: Damped Least Squares for Any Arm, Why It Needs the Damping, and What It Finds Instead of Eight

> When there is no formula, walk downhill with a step that knows when the ground is steep.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 28-03, 28-04

## The Problem

Lesson 28-04 solved the arm that was **designed to be solved**. The UR5 has an offset wrist; the Panda has seven joints, which is more than there is to solve for. For these, and for any arm somebody builds next, there is no formula. There is a method: **start somewhere, see how far the tool is from the target, and move the joints a little in the direction the Jacobian says will help; repeat.**

## The Concept

### The error is six numbers

How far is one pose from another? **Three** numbers for the place: the target less the present, in metres. **Three** for the orientation: the **rotation vector** (axis times angle) of the rotation that takes the present one to the target, `R target times R present transposed`. Orientation errors have the same traps as 28-02: an angle near a half turn has no single axis (handled with the columns of `(R + I) / 2`), and a tiny angle must not be lost in an arc cosine (the antisymmetric part is used).

### The step and the damping

The Jacobian gives `tool velocity = J times joint rates`. To get a joint step for an error, **invert** it: the plain way is the pseudo-inverse. Near a **singularity** (the elbow straight, where the arm cannot lengthen) it has a very small singular value and its inverse a very large one, so a small error demands an enormous step. With the elbow 0.002 rad from straight, the worst plain step for an error of 1 cm or 0.01 rad in any of the six directions was **20.61 rad**. (Exactly at the singularity the plain step is not large but **zero** in the direction that cannot be reached: nothing moves and the error does not shrink. The enormous steps are just beside it.) **Damping** adds `lambda squared` to the diagonal of `J J transpose` before the solve: the step is limited, at a price of a little accuracy in the directions the arm can move easily. With `lambda = 0.05` the step was **0.021 rad**. See `E-ARM-0013`.

The step is then also **limited** (no joint moves more than 0.3 rad in a step) and the error is **weighted** (a radian of rotation counts as half a metre), so that orientation and position are in the same units. The iteration ends when both errors are small, or after a number of steps, and **says which**.

### What it finds, and what it does not

From a start within 0.6 rad of the answer the UR5 reached the pose in **37 of 40** poses, in **21.4** steps on average. From a start of all zeros, **31 of 40**: it is a **local** method, and a start far from the answer can end in a place where the error cannot be improved. A solver that only ever started near its answer would look better than it is. See `E-ARM-0014`.

The Panda, with seven joints, reached the pose in **36 of 40**, and with **angles that were not the ones that made the pose**: there is a whole family of them (28-07 uses that). For the arm of 28-04, the iteration found **one of the eight** closed form solutions in **29 of 40** poses, and **which one depends on the start**. A pose out of reach was stopped by the **limit of steps**, with the tool **0.64 m** away and `ok` false: a solver that runs for ever is worse than one that gives up. See `E-ARM-0015`.

## Build It

`dls_step` in `exercise/solution.hpp`.

```
rcpp verify 28-05
```

## Use It

`rc/arm/ik_dls.hpp`.

```cpp
const rc::arm::IkResult r = rc::arm::ik_dls(table, target, base, tool, current_q, rc::arm::DlsConfig());
if (r.ok) command(r.q);
```

**What this does not do.** It is **local**: it finds the solution nearest the start, so the start should be the present joint angles, and it never says there is another. The damping is **fixed**: a better method raises it as the smallest singular value falls (28-06 measures that value). It clamps to the limits only by cutting the step, which can stall against a limit that a better solution would go round. It does not avoid obstacles, self collision or its own cables. Speed is not claimed: the Jacobian is rebuilt every step and the 6 by 6 solve is plain elimination. The error weights are one pair of numbers for every arm.

## What Breaks First

- **A step with no damping near a singularity.** See `E-ARM-0013`.
- **A solver tested only from near its answer.** See `E-ARM-0014`.
- **A solver that does not know it failed.** See `E-ARM-0015`.

## Ship It

`rc/arm/ik_dls.hpp` holds `rotation_vector`, `pose_error`, `solve6`, `dls_step`, `DlsConfig`, `IkResult` and `ik_dls`.
