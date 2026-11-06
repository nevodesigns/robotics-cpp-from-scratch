# What It Takes to Move It: Inertia That Changes and Joints That Pull on Each Other

> Holding an arm still takes its weight. Moving it takes its inertia, which changes
> with the pose, and terms where one joint's motion pulls on the other.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 13-06

## The Problem

Lesson 13-06 supplied the torque to hold an arm still, and a controller with that fed
forward holds any pose exactly. Ask the same arm to move quickly and it does not
follow: the tool lags and swings, and the elbow, told to stay where it is, is dragged
off as the shoulder passes.

Gravity is not the only thing the motors work against. The arm has inertia, and it is
not a number. It changes with the pose, and it couples the joints so that moving one
needs torque at the other.

## The Concept

### The mass matrix

For one joint, torque is inertia times acceleration, and the inertia is a number. For
two coupled joints it is a matrix:

```text
shoulder = M11 * a1 + M12 * a2
elbow    = M12 * a1 + M22 * a2
```

`M12` is the coupling: accelerating the shoulder takes torque at the elbow too. It is
symmetric, and it is positive definite, since a matrix that was not would describe an
arm that speeds up when pushed back.

Each entry is a mass times a distance squared, summed over the links: a mass far from
the axis is hard to swing. The elbow angle appears through a cosine, because that is
what folding the elbow does to the second link's distance from the shoulder. The
shoulder angle does not appear at all. Turning the whole arm does not change how its
parts are arranged, so it cannot change how hard they are to move.

### It is checked against the kinetic energy

The test does not trust the algebra. Kinetic energy is half of velocity times `M` times
velocity, so three velocities, (1, 0), (0, 1) and (1, 1), give all three entries. The
test computes that energy from the positions of the masses, moving each joint a little
and seeing how far every mass goes, and the links' own spin about their centres.

```text
39 elbow angles and payloads: the worst disagreement in any entry was 1.3e-10
```

### The inertia changes with the pose

The inertia the shoulder sees, as the elbow folds in, with a 0.5 kg payload:

| elbow angle | M11 (kg m^2) |
|---|---|
| 0.00, stretched | 1.176 |
| 1.05 | 0.956 |
| 2.09 | 0.516 |
| 3.14, folded flat | 0.296 |

Folded flat the arm is **4.0 times** easier to swing than stretched out. So a controller
tuned at one pose is a different controller at another. Damping goes as
`kd / (2 sqrt(kp * M))`, and a different `M` is a different damping with the same gains.

With gains of 60 and 12, a 0.3 rad shoulder step:

| elbow | M11 | overshoot | settled after |
|---|---|---|---|
| stretched | 1.176 | 1.9 % | 0.582 s |
| folded | 0.321 | none | 0.802 s |

Lightly damped where the arm is heavy and sluggish where it is light. The gains are
identical. See `E-KIN-0014`.

### The velocity terms

Once the arm moves, there are terms that exist only because it is moving: the Coriolis
and centripetal terms. A swinging shoulder pulls the elbow outward, and an elbow turning
while the shoulder turns adds a term of its own. Both come from one number:

```text
h = (m2 c2 + p l2) * l1 * sin(q2)

shoulder = ... - h * (2 v1 v2 + v2^2)
elbow    = ... + h * v1^2
```

It is zero when the arm is straight or folded flat, which is why a stretched arm has no
coupling and a bent one has a lot.

The whole torque for a motion is the three parts together:

```text
torque = M * acceleration + (the velocity terms) + the weight
```

and at rest, with no velocity and no acceleration, only the weight is left, which is
exactly the holding torque of lesson 13-06. The test checks that too.

### Velocity terms cannot be checked at rest

Weight and inertia can be tested standing still. The velocity terms are zero there, so a
sign wrong in one of them, or a term left out, is invisible to every static test and to
a slow move. There is a check that needs no second model.

With no motor torque and no friction, nothing adds energy to the arm or takes it away. A
free swing must keep its total of kinetic and potential energy, and that holds only if
the inertia, the weight and the velocity terms all agree. The test lets the arm swing
for five seconds from rest, hard enough to reach 38 rad/s at a joint, and measures how
far the total energy, 10.5 J at the start, wanders:

| model | drift |
|---|---|
| correct | 1.5e-06 J |
| both velocity terms with the wrong sign | 26.6 J |
| the elbow's squared velocity term left out | 151.8 J |

The wrong ones create more energy than the arm started with. See `E-KIN-0016`.

### What it buys

A shoulder swing of 1.0 rad in 0.4 s, with the elbow moving 1.4 rad, gains of 80 and 14.
The torque for the desired motion, from the desired trajectory, is added to the
controller's output, and the feedback corrects only what the model got wrong:

| feedforward | worst tool error | rms tool error |
|---|---|---|
| gravity only | 281.9 mm | 95.6 mm |
| gravity and inverse dynamics | **6.1 mm** | **2.0 mm** |

And with the elbow told to hold still while the shoulder swings, how far the swing
pulled it off:

| feedforward | elbow strayed |
|---|---|
| gravity only | 0.077 rad |
| gravity and inverse dynamics | 0.000 rad |

That is the coupling, seen. A controller that treats the joints as independent sees it
as a disturbance. See `E-KIN-0015`.

The trajectory has to supply the acceleration, so it has to have one: a quintic, which
starts and ends at rest with a continuous acceleration, and not a ramp.

## Build It

`mass_matrix` and `inverse_dynamics` in `exercise/solution.hpp`. The arm's body is from
lesson 13-06 and `LinkInertia` and `MassMatrix` are given. The comments write out each
entry.

```
rcpp verify 13-07
```

The first three tests are the matrix and the rest state. The fourth is the energy check,
the fifth the settling table, and the last the tracking and coupling tables, run on a
plant built from your functions, so a wrong one makes every number meaningless and the
energy test says so first.

## Use It

`rc/kin/dynamics.hpp` joins `rc::kin`, beside `gravity.hpp`.

```cpp
const rc::kin::JointTorques feed = rc::kin::inverse_dynamics(
    arm, inertia, d1, d2, w1, w2, z1, z2, payload);
shoulder_command = kp * (d1 - q1) + kd * (w1 - v1) + feed.shoulder;
```

**What this does not do.** It is a model, and the feedback is there for what the model
gets wrong. Link masses, centres and inertias are known to a few percent at best for a
real arm, and friction, backlash, the motor's own rotor inertia seen through a gear
ratio, and flexing in the links are not in it. The simulation is the arm exactly as
modelled, so the 6.1 mm is the best case. On hardware the feedforward is worth less and
the feedback worth more, and how much is a measurement to make on the arm.

It is also two joints in a plane. Six joints in space have the same structure and a mass
matrix that depends on more of the pose, which is what a library such as Pinocchio
computes, and it is the right tool once the arm has more than two links.

## What Breaks First

- **One set of gains for every pose.** The damping changes with the inertia. See
  `E-KIN-0014`.
- **Independent joint control.** The inertia and the coupling are left to the feedback.
  See `E-KIN-0015`.
- **A model checked only at rest.** The velocity terms are zero there. See
  `E-KIN-0016`.

## Ship It

`rc/kin/dynamics.hpp` holds `LinkInertia`, `MassMatrix`, `mass_matrix` and
`inverse_dynamics`.

Phase 13 now has the arm asked in both directions, told apart the pose it cannot reach
from the one it cannot hold, and its dynamics: what it costs to hold it and what it
costs to move it.
