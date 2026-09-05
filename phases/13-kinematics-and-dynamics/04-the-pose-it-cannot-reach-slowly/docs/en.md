# The Pose It Cannot Reach Slowly: Singularities and What They Cost

> A tenth of a millimetre from full extension, moving the tool ten centimetres a
> second needs the elbow at a hundred and forty revolutions a minute. A tenth of
> that distance closer, ten times as much.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 13-03, 06-05

## The Problem

Lesson 13-03 built a Jacobian by measuring, and used it to reach a pose with no
formula. It worked, and it will keep working right up until the arm goes
somewhere the Jacobian cannot be inverted.

There are places in every arm's workspace where the tool loses a direction: a
way it cannot move at all, whatever the joints do. Approaching one of them, the
joint rates needed to keep the tool moving grow without bound, and the path that
goes through it looks perfectly ordinary on a screen.

## The Concept

### Where the trouble is, exactly

For a two link planar arm the Jacobian has a closed form, and its determinant
works out to

```
det J = l1 * l2 * sin(q2)
```

which depends on the **elbow** and on nothing else. Largest with the elbow
square; zero with the arm straight, either fully extended or folded back on
itself. Where the arm is pointing does not come into it.

That is worth having next to the measured Jacobian from 13-03. The general case
has no formula and must be measured; this one has one, and it says exactly where
the trouble is rather than leaving you to discover it.

### What it costs on the way in

A 0.5 and 0.4 metre arm, tool moving outward at 0.1 m/s:

| reach | elbow | det J | q1 rate | q2 rate |
|---|---|---|---|---|
| 0.5000 | 1.9823 | 0.183303 | 0.09 | -0.27 |
| 0.7000 | 1.3694 | 0.195959 | 0.15 | -0.36 |
| 0.8500 | 0.6741 | 0.124844 | 0.30 | -0.68 |
| 0.8800 | 0.4251 | 0.082481 | 0.47 | -1.07 |
| 0.8950 | 0.2122 | 0.042129 | 0.94 | -2.12 |
| 0.8990 | 0.0949 | 0.018947 | 2.11 | -4.74 |
| 0.8999 | 0.0300 | 0.005999 | **6.67** | **-15.00** |

Every one of those rates does produce exactly the motion asked for. The arm is
not failing; it is doing what was requested, at whatever speed that takes.

At full extension it is not large, it is **impossible**. The tool cannot move
outward at any joint speed at all. Sideways is still available, and that is what
"loses a direction" means.

So the solver refuses rather than dividing:

```cpp
if (std::fabs(det) <= least_determinant) return JointRates{};
```

A solver that inverts regardless produces a number, and that number goes to the
motors.

### Two answers that become one

The same pose has more than one set of joint angles, and near a singularity they
converge:

| reach | elbow up | elbow down | apart |
|---|---|---|---|
| 0.50000 | 1.982313 | -1.982313 | **3.964626** |
| 0.80000 | 0.958192 | -0.958192 | 1.916384 |
| 0.88000 | 0.425094 | -0.425094 | 0.850188 |
| 0.89990 | 0.030000 | -0.030000 | 0.060001 |
| 0.89999 | 0.009487 | -0.009487 | **0.018974** |

Two completely different postures, four radians apart with the arm half out and
two hundredths of a radian apart near the limit.

At that separation the choice is decided by a rounding error or a sensor's last
bit. Both answers put the tool exactly where it was asked, so nothing in the
solver has anything to complain about, and the arm swings through half its
workspace to satisfy a request that moved the tool a millimetre.

Choose the branch deliberately: the solution nearest the current pose, carried
as state for the whole motion rather than recomputed per point.

### Damping, and what it actually buys

Damped least squares always has an answer. Instead of solving `J q = v` exactly
it minimises the tool error and the joint effort together, and the matrix it
inverts is `J J' + lambda^2 I`, which cannot be singular.

At a tenth of a millimetre from full extension, asked for 0.1 m/s:

| lambda | q1 rate | q2 rate | speed given |
|---|---|---|---|
| 0.000 | 6.67 | -15.00 | 0.10000 |
| 0.001 | 6.49 | -14.61 | 0.09738 |
| 0.010 | 1.80 | -4.06 | **0.02707** |
| 0.050 | 0.10 | -0.22 | 0.00149 |
| 0.200 | 0.01 | -0.01 | 0.00028 |

Every step costs joint rate and costs speed, together. At lambda 0.01 the arm
moves at a quarter of the rate and delivers a quarter of the motion.

**Damping does not get the arm through the singularity.** It makes the arm
refuse to go there quickly, which is exactly why a real machine slows to a crawl
near full extension rather than throwing itself at the stop.

Two consequences.

**Leave it switched on.** At half reach with lambda 0.01 the tool still moves at
over 99 percent of the speed asked for, so there is no need to switch it in and
out and one fewer mode to get wrong.

**Report the shortfall.** `J q` against `v` is the difference between what was
asked and what the joints will deliver, and it is the number that tells a path
follower it is falling behind rather than tracking. Then slow the whole path in
time, because slowing one direction and not the others distorts the shape being
drawn.

## Build It

Implement `planar_jacobian`, `joint_rates` and `damped_joint_rates` in
`exercise/solution.hpp`.

```
rcpp verify 13-04
```

The suite checks the closed-form Jacobian against a central difference of the
tool position, sweeps the arm toward full extension watching the joint rates,
measures how far apart the two solutions are, and then sweeps the damping.

## Use It

**Keep a distance measure alongside the solver.** `manipulability` is free for
this arm and is `sqrt(det(J J'))` for a general one, computed from the Jacobian
you already have.

**Plan a workspace smaller than the reach.** A path that passes near full
extension can usually be moved a few centimetres in, and this is why
manufacturers quote a usable envelope as well as a reach.

**Choose lambda from the joint rate you can afford**, which is on the data
sheet, rather than by feel. One loop produces the table.

**Test at the edge.** A path that stays in the middle of the workspace exercises
none of this.

## What Breaks First

- **Joint rates that go to infinity.** See `E-KIN-0005`.
- **Two postures that become indistinguishable.** See `E-KIN-0006`.
- **Damping used as though it removed the problem.** See `E-KIN-0007`.

## Ship It

`planar_jacobian`, `manipulability`, `joint_rates` and `damped_joint_rates` join
`rc::kin` beside the chain and the reach solver. The arm can now be asked to
move at a speed and answer whether it is able to, which is the difference
between a solver and something you would let near a machine.
