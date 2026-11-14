# A Hand That Pinches: Stroke, Force, Friction, and How Heavy a Thing Two Fingers Can Hold

> A grip does not hold a thing by squeezing hard; it holds it by friction, and friction is a number you can work out.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 29-06, 28-02

## The Problem

Phases 28 to 30 got the arm's hand to a place without hitting anything. A hand has to pick something up, and what holds a thing in two fingers is not the squeeze alone but the friction the squeeze makes at the pads. A grip that is too weak lets the thing slip, and one that is too strong crushes it and heats the motor. The first question is a number: how hard to squeeze.

## The Concept

### A hand as numbers

A parallel gripper has a stroke (how far it opens), a force range, a finger speed and the friction between pad and thing. The model here has a stroke of 0.085 m and a squeeze of up to 235 N; these are typical of a hand of this class (a published hand's own sheet gives its numbers, and the lesson's say "typical"). See `E-ARM-0076`.

### What friction holds

Each pad can resist a pull along it of at most mu times the squeeze. Two pads give 2 mu F. To hold a mass the weight, m times (g plus the acceleration), must be at most that, and a safety factor goes on top: F = safety m (g + a) / (2 mu). A kilogram at rest took 9.81 N at a friction of 0.5; lifted at 5 m/s2 it took 14.81 N, because the hand's acceleration adds to the weight. See `E-ARM-0077`.

### The heaviest thing

Turned round: the heaviest mass is 2 mu F over safety (g + a). With the full squeeze and a safety factor of 1.5 the hand held at most 16.0 kg; with pads at half the friction (wet or oily) it held 8.0 kg. Friction is the part that is not known: a pad that is dirty is a smaller hand. See `E-ARM-0078`.

### Does it fit, and how long does it take

A width must lie between closed and open. Closing takes the distance over the finger speed, which is a part of every pick's time.

## Build It

`required_force` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- The weight is mass times (gravity plus the acceleration); two pads share it, each holding mu times the squeeze; times the safety factor.

As shipped, 2 of the 3 tests fail, and these are the ones your code has to turn green:

- The force that holds a mass is the weight over the friction of two pads, and grows with the acceleration.
- The heaviest payload is the same sum turned round, and halving the friction halves it.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 31-01
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/arm/gripper.hpp`.

```cpp
rc::arm::Gripper hand;
const double f = rc::arm::required_force(hand, mass, accel, 1.5);
if (f > hand.f_max) refuse_the_pick();
```

**What this does not do.** Friction is a single coefficient and the pads are a point contact each; a real grip has a pad that deforms, a thing that is not a box, and a friction that depends on speed and on what is on the pads. The two fingers move together (a parallel hand). The squeeze is commanded, not measured: whether it is reached is a matter for the contact lessons later in the phase.

## What Breaks First

- **Squeezing for the weight at rest and lifting fast.** See `E-ARM-0077`.
- **A friction taken from a clean sheet and used on a dirty pad.** See `E-ARM-0078`.
- **A hand chosen by its stroke alone.** See `E-ARM-0076`.

## Ship It

`rc/arm/gripper.hpp` holds `Gripper`, `required_force`, `max_payload`, `fits`, `close_time` and `grip_margin`.
