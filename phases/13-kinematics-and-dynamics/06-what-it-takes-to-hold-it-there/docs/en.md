# What It Takes to Hold It There: Gravity and the Motors

> An arm is held up by its motors, and how hard they push depends on where the arm
> is. Work out the torque, then see what a controller does without it.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 13-05

## The Problem

The first five lessons of this phase treated the arm as geometry: where the tool is,
which angles put it there, which of them the joints can hold. None of that has weight.

Give the arm links with mass and a payload in the gripper, tell a proportional
controller to hold it stretched out, and it settles about a third of a metre below
the pose it was told to take. The controller reports no fault. It is doing its job:
it supplies torque in proportion to an error, so to supply the torque gravity asks
for, it has to have an error.

This lesson works out the torque, and measures what ignoring it costs.

## The Concept

### The torque is a cosine of the pose

Every link's weight acts at its centre of mass, and a joint feels it through the
horizontal distance from the joint to that point. That distance is the length times
the cosine of the angle the link makes with the horizontal, so the torque is the
weight times that cosine: largest with the link horizontal, zero when it is vertical,
and reversed beyond.

For the planar arm of lesson 13-02, in a vertical plane, with `q12 = q1 + q2`:

```text
elbow     = (m2 c2 + p l2) g cos(q12)
shoulder  = (m1 c1 + (m2 + p) l1) g cos(q1) + elbow
```

The elbow carries what is beyond it: the second link at its centre of mass `c2`, and
the payload `p` at the tool. The shoulder carries all of that, plus the first link,
plus the whole weight of the second link and the payload hanging at the elbow.

Two things in it are easy to get wrong. The elbow torque depends on the angle the
*second link* makes with the horizontal, which is `q1 + q2`, and on neither alone:
the same elbow torque appears at (0.3, 0.5) and at (0.8, 0.0). And the sign flips once
the arm leans back past the vertical: the torques at 0.3 rad and at pi minus 0.3 rad
are equal and opposite. See `E-KIN-0013`.

### Checking it against something that shares no algebra

The test does not trust the formula. The torque needed to hold a pose still is the
slope of the potential energy with respect to each joint, so the test computes the
energy of every mass, differentiates it numerically, and compares.

```text
507 poses and payloads: the worst disagreement was 4.8e-09 N m
```

A check that uses the same algebra as the code would pass with the same mistake in
both. This one cannot.

### What it costs to ignore

A 0.5 m and 0.4 m arm, 0.5 kg in the gripper, a proportional gain of 50 N m per radian
on each joint, ten simulated seconds to settle. How far the tool ends up from where it
was told to be:

| pose | shoulder torque needed | proportional only | torque fed forward |
|---|---|---|---|
| straight out | 17.56 N m | **326.6 mm** | 0.0000 mm |
| raised 45 degrees | 12.42 N m | 314.7 mm | 0.0000 mm |
| elbow bent | 6.31 N m | 89.8 mm | 0.0000 mm |
| straight up | 0.00 N m | 0.0 mm | 0.0000 mm |

The sag is the torque over the gain, so it follows the torque: biggest where the arm
is stretched out, and nothing at the upright pose a bench demonstration usually uses.
The controller is not faulty and raising the gain only shrinks the miss.

Feeding the holding torque forward, computed from the pose the arm is told to take,
removes it. That is a feedforward, which phase 14 builds a whole lesson on, with a physical
meaning here, and it works before the arm drops, where an integral term removes the
sag after it has happened and with the windup that phase 14 measures. See `E-KIN-0011`.

### The heaviest payload is a property of the pose

The torque is a straight line in the payload: `alone + per_kg * p`. So the heaviest
payload a joint can hold is solved for, not searched for, and the answer for the arm
is the smaller of the two joints.

Motors of 20 N m at the shoulder and 8 N m at the elbow, on that arm:

| pose | heaviest payload |
|---|---|
| straight out | 0.776 kg |
| raised 45 degrees | 1.715 kg |
| elbow bent | 2.283 kg |
| straight up | no limit |

A rating of 2.2 kg is true with the elbow bent and nearly three times too high for the
pose with the longest lever. A payload rating quoted without a pose is the best case.
See `E-KIN-0012`.

Two details. An arm that cannot hold itself can hold nothing, and the answer is zero.
And standing straight up neither joint feels the payload at all, which is `cos(pi / 2)`:
that is 6e-17 and not zero, so it is compared with a tolerance. Without one, a pose
that should be unlimited reports a payload in the millions.

### Upright is a hilltop

Standing up, the torque falls as the arm leans further, so gravity adds to any lean
instead of resisting it. Measured from the holding torque itself, it pushes the
shoulder over by **17.6 N m for every radian**. A proportional gain below that cannot
bring the arm back.

The arm standing up, nudged 0.02 rad, with no feedforward:

| gain | where it ends up |
|---|---|
| 10 | 3.28 rad, fallen over |
| 20 | 1.79 rad |
| 50 | 1.60 rad, held |

A gain that is fine for an arm hanging down is not enough for the same arm standing
up. Choose it against the largest slope gravity reaches at the poses the arm will
hold.

## Build It

`holding_torques` and `heaviest_payload` in `exercise/solution.hpp`. The body of the
arm and the result type are given, and the comments say what each weight acts through.

```
rcpp verify 13-06
```

Five tests are about the formula and the payload. The last two are the simulations
above, a joint space controller on the arm with gravity taken from the energy, so the
sag depends on the physics and the feedforward row depends on your function.

## Use It

`rc/kin/gravity.hpp` joins `rc::kin`.

```cpp
rc::kin::ArmBody arm;
const rc::kin::JointTorques hold = rc::kin::holding_torques(arm, q1, q2, payload);
shoulder_command = pid_shoulder.update(error) + hold.shoulder;
```

**What this does not do.** It is statics: the torque to hold still. Moving the arm
needs more: the inertia of the links, which changes with the elbow angle, and the
Coriolis and centripetal terms that couple the joints once it moves. The simulation here
uses a constant inertia and ignores that coupling, which is fine for where the arm
*ends up* and wrong for how it gets there. Friction, gear ratios and the motor's torque
constant are also not modelled: a torque in newton metres is what the joint needs, and
what the motor driver is commanded is another number.

## What Breaks First

- **Holding with a proportional gain alone.** It sags by the torque over the gain. See
  `E-KIN-0011`.
- **A payload rating without a pose.** It is the best case. See `E-KIN-0012`.
- **Gravity as a constant, or the wrong angle, or a low gain at the top.** It is a
  cosine and it reverses. See `E-KIN-0013`.

## Ship It

`rc/kin/gravity.hpp` holds `ArmBody`, `JointTorques`, `holding_torques` and
`heaviest_payload`.

The torque to hold still is the first half of dynamics. The second is what it takes to
move, and why a controller tuned for one pose is wrong at another.
