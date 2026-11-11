# How Long the Move Takes: Trapezoid Profiles, the Triangle That Replaces Them, and Joints That Must Arrive Together

> A move needs a time. Ramp up, go level, ramp down; for a short move the ramps meet; and joints that move together must be given times that make them finish together.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 13-09, 13-05

## The Problem

A path says where an arm goes and not when. Told to be somewhere else at once, a joint is asked for infinite acceleration, and the motor trips or the arm shakes. A move has to have a time that respects what the motor can do, and an arm has several joints with different distances to go, which have to finish together or the tool wanders on the way.

## The Concept

### A trapezoid

The speed against time is a ramp up at the greatest acceleration, a level top at the greatest speed, a ramp down. For a distance of 2 with a speed limit of 1 and an acceleration limit of 2: ramps of **0.5** s, a cruise of **1.5** s, a total of **2.5** s. The position at 0.25 s was 0.0625 (0.5 times 2 times 0.25 squared), at 0.5 s was 0.25 (the end of the first ramp), at 1.0 s was 0.75, at 2.0 s was 1.75, at 2.5 s was 2.0, and after the end or before the start it stayed. See `E-KIN-0035`.

### A triangle

A move of 0.25 never reaches the speed limit: the ramps meet before the top. The formula for the long move applied to it gives a cruise of less than nothing. It is a triangle with a peak of the square root of (acceleration times distance), **0.707107**, and a total of **0.707107** s, with half the distance covered at half the time. The boundary, a distance of exactly twice the ramp distance, is a triangle that just reaches the limit: 0.5 took 1.0 s with a peak of 1. See `E-KIN-0036`.

### Inside the limits

Nine moves, long and short and in both directions, sampled along their length and differenced: **0** went over the speed limit, **0** over the acceleration limit, **0** steps went backwards, and **0** ended anywhere but the distance asked for.

### Arriving together

Two joints with distances 2 and 0.5 and the same limits took **2.5** s and **1.0** s alone. In a move that finishes at different times the faster joint is done while the other is still going, and the tool leaves its path. `synchronise` gives every joint the time of the slowest, 2.5 s, and the faster one goes at a lower peak with the same ramps: (aT less the root of a squared T squared less 4 a d) over 2, which is **0.208712** instead of 1. Both were at their targets at 2.5 s, and the second was not there already at 1.0 s. See `E-KIN-0037`.

### The edges

A joint that does not move has no time and stays at 0; a negative distance is the mirror; limits that are not positive are not a move, and for a set of joints they make the duration 0. Three joints, one of which does not move, took the time of the other two.

## Build It

`plan_trapezoid`, `position_at` and `synchronise` in `exercise/solution.hpp`.

```
rcpp verify 13-14
```

## Use It

`rc/kin/timing.hpp` joins its module.

```cpp
const rc::kin::Synchronised move = rc::kin::synchronise({dq1, dq2, dq3}, {1.5, 1.5, 3.0}, {4.0, 4.0, 8.0});
for (double t = 0.0; t <= move.duration; t += 0.002) command(q0 + position_at(move.joints[0], t), /* ... */);
```

**What this does not do.** It makes joints arrive together and not move in a straight line: the joints are at different fractions of their distances along the way, so the tool is on a curve between the two ends (lesson 13-09 is about when that matters), and a straight line in joint space needs one profile scaled to all the joints. The acceleration is a step at the corners of the trapezoid, which is a jerk, and a smoother profile limits that as well. The limits of a joint are what you give it, and a limit on the speed of the tool, or on the torque a move asks for (lesson 13-07), is another matter. And it plans from rest to rest: a move that continues into another does not stop in between.

## What Breaks First

- **A move with no time.** See `E-KIN-0035`.
- **A short move with a long move formula.** See `E-KIN-0036`.
- **Joints that finish at different times.** See `E-KIN-0037`.

## Ship It

`rc/kin/timing.hpp` holds `Trapezoid`, `plan_trapezoid`, `position_at`, `Synchronised` and `synchronise`.

The capstone times the move it has decided the arm can make.
