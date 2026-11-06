# What the Tool Can Push: Force Is the Dual of Speed

> The Jacobian says how fast the tool can move. Turned over, it says how hard it can
> push, and the directions it is fast in are the ones it is weak in.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 13-04, 13-06

## The Problem

An arm that opens a door, tightens a screw, or holds a part against a grinder has a
different job from one that moves. Its tool applies a force, and the motors have to supply
the torques that hold it. How hard the tool can push depends on where the arm is and which
way it pushes, and a single number on a datasheet hides all of that.

The arm that is rated for 20 N pushes a door open in one place and cannot move it in another,
with the same motors.

## The Concept

### The same matrix, turned over

Lesson 13-04's Jacobian maps joint speeds to tool speed: `v = J * qdot`. Force goes the other
way, and the matrix is the transpose.

Power is the same at both ends of a rigid arm. The torques dotted with the joint speeds equal
the force dotted with the tool speed. Since tool speed is `J * qdot`, that makes torque `J^T`
times force:

```text
shoulder = a * fx + c * fy
elbow    = b * fx + d * fy        where J = [a b; c d]
```

Written with `J` and not `J^T`, the first row is unchanged and the second is not, which is why
the mistake survives a casual look. Holding 10 N along x at a pose of (0.4, 1.1):

| | shoulder | elbow |
|---|---|---|
| with the transpose | -5.94 N m | -3.99 N m |
| with the matrix as it is | -5.94 N m | **+4.89 N m** |

The shoulder is identical, and the elbow has the wrong sign. See `E-KIN-0017`.

### Checked by the work the force does

The test does not trust the algebra. Move each joint a little, see how far the tool goes, and
the torque at that joint is the work the force does per radian: the principle of virtual work.
The test does that numerically, with a position computed from the geometry and nothing from the
Jacobian, across 1014 poses and forces.

```text
the worst disagreement was 1.9e-09 N m
```

### Going back, and where it is blind

The force at the tool that a pair of joint torques amounts to is the inverse of the transpose,
so it needs the determinant, which is `l1 * l2 * sin(q2)`. At full extension it is zero. A force
along a straight arm puts no torque on either joint, because the load goes straight down the
structure, so the torques contain no information about it and the inverse does not exist.

Near it the inverse exists and is badly conditioned. With the elbow at 0.01 rad, a change of 0.1
N m in one torque changes the force it implies by **20.0 N**. A motor current is noisier than
that. A robot that estimates the force at its gripper from its motor currents goes blind as the
arm straightens, and `force_for_torques` says so with a flag and does not hand back a division by
almost zero. See `E-KIN-0018`.

### The hardest it can push

A unit of force in a direction costs a torque at each joint, and the joints each have a limit, so
the push can be scaled until the first reaches it. That is `strongest_push`, and it puts exactly
one joint on its limit, which the test checks over 24 directions.

Motors of 20 and 8 N m, joints held to 2 rad/s, with the first link at 0.4 rad:

| elbow | push along | push across | move along | move across |
|---|---|---|---|---|
| 0.05 | 720.1 N | 20.0 N | 0.02 m/s | 1.80 m/s |
| 0.50 | 72.8 N | 20.8 N | 0.22 m/s | 1.74 m/s |
| 1.00 | 37.6 N | 23.6 N | 0.43 m/s | 1.58 m/s |
| 1.57 | 25.6 N | 31.2 N | 0.62 m/s | 1.28 m/s |
| 2.50 | 20.0 N | 66.8 N | 0.80 m/s | 0.60 m/s |

*Along* is the direction the arm points, from the shoulder to the tool. *Across* is square to it.

Nearly straight, the arm can push along itself **36 times** harder than across itself and moves
along itself **81 times** slower. The direction it cannot move in is the direction it can push
hardest. That is not a coincidence: it is the same Jacobian, and where it squeezes one it
stretches the other. Exactly straight, a push along the arm costs no torque at all, and the answer
is no limit.

Across the arm the limit is the motors, at the full reach times the force: `20 / (0.5 + 0.4)` at
the shoulder and `8 / 0.4` at the elbow, and the smaller of those, 20 N from the elbow. See
`E-KIN-0019`.

## Build It

`torques_for_force`, `force_for_torques` and `strongest_push` in `exercise/solution.hpp`. The
Jacobian is the one from lesson 13-04. The comments give the matrices.

```
rcpp verify 13-08
```

## Use It

`rc/kin/force.hpp` joins `rc::kin`, beside `singularity.hpp`.

```cpp
const rc::kin::PlanarJacobian j = rc::kin::planar_jacobian(0.5, 0.4, q1, q2);
const double limit = rc::kin::strongest_push(j, dx, dy, 20.0, 8.0);
if (wanted_force > limit) plan_another_pose();
```

**What this does not do.** It is statics: the arm holds still against a force. A tool that is
pushing while moving needs the dynamics of lesson 13-07 as well, and weight is separate again,
the holding torque of lesson 13-06, to be added to the torque for the force. The limits are the
motors' continuous limits as a constant; a real motor's limit depends on temperature and speed.
And the force is taken at the tool tip in the plane: a tool with an offset, or a force with a
moment, needs a wrench, which is a force and a torque together, and a Jacobian with six rows.

## What Breaks First

- **The Jacobian used as it is, not transposed.** The elbow torque comes out the wrong way. See
  `E-KIN-0017`.
- **A force estimated from torques near a straight arm.** It is blind there. See `E-KIN-0018`.
- **A strength given as one number.** It depends on the direction and the pose. See
  `E-KIN-0019`.

## Ship It

`rc/kin/force.hpp` holds `ToolForce`, `JointLoad`, `torques_for_force`, `force_for_torques` and
`strongest_push`.

Lesson 13-04 found that the direction the arm cannot move in is a problem. This finds that the same
direction is a gift when the job is to push.
