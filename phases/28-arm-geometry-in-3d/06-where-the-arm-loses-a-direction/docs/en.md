# Where the Arm Loses a Direction: Singular Values, Three Kinds of Singularity, and a Line Drawn Over the Shoulder

> At a singularity the arm cannot move one way however hard it tries, and just beside it, it moves the other ways at any speed you ask.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 28-05, 13-04

## The Problem

Lesson 28-05 gave the step a damping so that it would not be enormous near a singularity, and said little about where the singularities are, or how near one is. An arm in a singular pose has lost a direction: the tool cannot move one way, however fast the joints turn. And on the approach it is worse than at the pose itself: the tool can move that way, but only for an enormous joint speed. A planner has to know which poses to keep away from and by how much.

## The Concept

### How near: the smallest singular value

The Jacobian maps joint rates to tool velocity. Its singular values are the gains of that map in the six directions in which it is most and least responsive (the square roots of the eigenvalues of `J J transpose`, which a small Jacobi routine finds). The smallest is the gain in the worst direction: how much tool velocity you get in that direction for a unit of joint rate. When it is zero, the direction is gone. The product of them (manipulability) and the ratio of the largest to the smallest (the condition number) are other ways of reading the same thing.

### The three kinds

For an arm with an elbow and a spherical wrist the smallest singular value was 0.172 in a generic pose, and zero (to a millionth: 0.00000000, 0.00000000, 0.00000000) in each of three families:

- **Wrist:** joint 5 at zero, so joints 4 and 6 turn about the same axis: the tool cannot rotate about the axis they have lost.
- **Elbow:** the arm stretched (or folded) so the wrist centre cannot go further along the line of the arm.
- **Shoulder:** the wrist centre on the axis of the first joint, where turning the first joint moves the tool nowhere.

As joint 5 went from 0.5 to 0.1 to 0.01 rad the smallest singular value fell from 0.1337 to 0.0313 to 0.00317: a smooth fall, which means there is no cliff to see coming; the number tells you how far you are. See `E-ARM-0016`.

### A line past the shoulder

The classic: draw a straight line with the tool, orientation fixed, that passes close to the shoulder axis. To follow it, joint 1 has to turn through a half turn as the tool goes by, and the nearer the line is to the axis the faster. The line passed 0.01 m from the axis, at 0.25 m/s: an undamped resolved rate step turned joint 1 at 26.1 rad/s, which is faster than any arm's joint can; a step damped only as much as the smallest singular value says (none above a threshold, rising to a maximum at zero: `adaptive_lambda`) turned it at 2.0 rad/s. The price: the damped step let the tool stray 0.0448 m from the line, against 0.0006 m. That is the trade, and it is the right one to make on purpose: a slow, slightly wrong pass beats an impossible, exact one. See `E-ARM-0017`.

### Planning around it

The better answer is not to go near. A path planner (phase 30) that keeps the smallest singular value above a floor (or the manipulability above one) has no singularity to damp. And the wrist singularity in particular can be avoided by choosing the other of the two wrist solutions of 28-04, or by choosing the tool's orientation so that joint 5 stays well away from zero. See `E-ARM-0018`.

## Build It

`adaptive_lambda` in `exercise/solution.hpp`.

```
rcpp verify 28-06
```

## Use It

`rc/arm/singular.hpp`.

```cpp
const rc::arm::Jacobian j = rc::arm::jacobian(table, q, base, tool, table.convention);
const double lambda = rc::arm::adaptive_lambda(rc::arm::smallest_singular(j), 0.1, 0.1);
```

**What this does not do.** The smallest singular value mixes metres and radians (the linear rows and the angular rows are in different units), so its number depends on how the rows are scaled; the weights of 28-05 and the `eps` here are choices. It says how near a singularity is and not which direction is lost (that is the singular vector, which this does not return). The three kinds were found for one arm; another layout has others (a Panda has a family of its own, and with seven joints a singularity is a place the extra joint may be able to leave). Joint limits are not in it: a pose can be well away from a singularity and against a stop. The line test is for one line and one arm.

## What Breaks First

- **A pose near a singularity that nobody checked.** See `E-ARM-0016`.
- **A resolved rate controller with no damping.** See `E-ARM-0017`.
- **A path that goes through the wrist's singularity.** See `E-ARM-0018`.

## Ship It

`rc/arm/singular.hpp` holds `singular_values`, `smallest_singular`, `manipulability`, `condition_number`, `adaptive_lambda`, `RateRun`, `RateConfig` and `resolved_rate_line`.
