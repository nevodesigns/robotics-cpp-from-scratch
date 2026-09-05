# A Profile the Machine Can Follow: Where the Setpoint Comes From

> It arrived at exactly the right place, having asked for an infinite
> acceleration on the way. The endpoint test passed.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 14-05, 13-04

## The Problem

Every lesson so far has handed the loop a setpoint and moved on. This one asks
where the setpoint comes from.

The obvious answer is: from wherever you want the robot to be. Write the target
position and let the loop deal with it. That is a **step**, and it tells the loop
to be somewhere else immediately, which is a request for an infinite velocity.

Lesson 14-05 also needs something a step cannot supply: a velocity and an
acceleration to feed forward. A profile has both, because it computed them.

## The Concept

### What a step costs

A one metre move on the loop from 14-05, as a step and through a profile:

| | peak command | worst error |
|---|---|---|
| a step | **20.2 N** | 0.9223 m |
| a profile with feedforward | **1.4 N** | 0.1987 m |

Fourteen times the command. And the peak is set by the **gain**, not by anything
about the machine or the move: raise kp and the bang gets louder.

The gains are not at fault. They were chosen in 14-04 for a loop that has to
react to disturbances, and a step turns every commanded move into one.

What is left of the profile's error is exactly the standing lag from 14-05,
`kd * v / kp = 0.2`, which that lesson says how to remove.

### The shape

Speed up at the acceleration limit, hold the top speed, slow down again.

| distance | ramp | cruise | total | peak | arrives at |
|---|---|---|---|---|---|
| 0.050 | 0.2236 | 0.0000 | 0.4472 | 0.2236 | 0.050000000 |
| 0.100 | 0.3162 | 0.0000 | 0.6325 | 0.3162 | 0.100000000 |
| 0.200 | 0.4472 | 0.0000 | 0.8944 | 0.4472 | 0.200000000 |
| **0.250** | 0.5000 | **0.0000** | 1.0000 | 0.5000 | 0.250000000 |
| 0.300 | 0.5000 | 0.1000 | 1.1000 | 0.5000 | 0.300000000 |
| 1.000 | 0.5000 | 1.5000 | 2.5000 | 0.5000 | 1.000000000 |

Look at the cruise column. Above 0.25 m there are three phases; below it there
are two, and the peak speed is never reached.

The boundary is exact: `top_speed^2 / acceleration`, which is what speeding up
and slowing down again costs. Below that the profile is a triangle, and its peak
is `sqrt(acceleration * distance)`.

### The short move, which is most moves

A planner that assumes three phases regardless does not crash. It computes a
**negative** cruise time and carries on. With a 0.10 m move:

```
  ramp 0.5000 s, cruise -0.3000 s, duration 0.7000 s
```

| t | naive velocity | correct velocity |
|---|---|---|
| 0.3000 | 0.300000 | 0.300000 |
| 0.4000 | 0.400000 | 0.232456 |
| 0.4999 | **0.499900** | 0.132556 |
| 0.5001 | **0.199900** | 0.132356 |
| 0.6000 | 0.100000 | 0.032456 |

The commanded velocity drops by 0.3 m/s between two adjacent instants. That is
an infinite acceleration, and it is exactly what a profile exists to prevent.

And it arrives at 0.100000 m with a velocity of exactly zero. **The endpoint
test passes.** Every fault is in the middle.

So the test has to look at the middle:

```cpp
// no two adjacent samples may differ by more than the profile's own
// acceleration over the interval between them
RC_CHECK(jump <= acceleration * interval * 1.001);
```

One loop, and it needs a move shorter than `top_speed^2 / acceleration`, which
for most machines is a few centimetres, and which is most of the moves a robot
actually makes.

### Slowing without distorting

Lesson 13-04 ended by saying that when a singularity limits how fast one
direction can be driven, the whole path should slow rather than that direction.
This is what that means.

```cpp
const Trapezoid slower = profile.scaled_to(seconds);
```

Stretching time by a factor divides every velocity by it and every acceleration
by its square. Measured, a one metre profile stretched to twice its duration has
half the peak speed and is at the **same position at every fraction of the way
through**.

The shape of a path is the relationship between its axes at each instant. Scale
one axis and that relationship changes, which is a different path that happens
to share its endpoints. For several axes: compute the time each needs alone, take
the longest, stretch them all to it.

And refuse a duration shorter than the profile's own. The machine cannot do it,
and handing back something it cannot follow only moves the failure downstream to
somewhere with less information about why.

### The two cases that crash

**A move of zero.** No distance, no direction, and dividing by either is the
usual way this fails on the day a target happens to equal the current position.

**A move backwards.** The same profile with every sign turned round. Writing it
as a positive distance and a separate direction keeps the arithmetic in one
place; getting it wrong produces a machine that works in one direction, which
people discover by driving into something.

## Build It

Implement the constructor, `at` and `scaled_to` in `exercise/solution.hpp`.

```
rcpp verify 14-06
```

The suite plans seven distances across the boundary, samples a profile twenty
thousand times looking for a jump, plans a short move both ways, stretches one,
and drives a metre through the loop from 14-05 as a step and as a profile.

## Use It

**Never step a setpoint at a machine.** Profile it, even for a small move,
especially for a small move.

**Take the limits from the machine.** Top speed from the drive's rating,
acceleration from what the mechanism and the payload tolerate. Both are
measurable and both belong in the source with a note saying where they came
from.

**Pass the velocity and acceleration on** to the feedforward from 14-05. They
are already computed and they are most of what makes the tracking good.

**Use the duration.** It is known before anything moves, so it can be planned
around, coordinated against, or refused.

**Test the middle.** Endpoints agree even when everything between them is wrong.

## What Breaks First

- **A setpoint that steps.** See `E-CTRL-0012`.
- **Three phases on a two phase move.** See `E-CTRL-0013`.
- **Slowing one axis instead of the move.** See `E-CTRL-0014`.

## Ship It

`Trapezoid` joins `rc::control` beside the PID, the tuning tools and the
feedforward model. The follower in 16-04 now has something to hand it, the
singularity check in 13-04 has somewhere to send its speed limit, and every
commanded move from here is one the machine could have made on its own.
