# Putting the Path in Time: Jerk Limited Profiles That Respect Every Joint, the Stop at Every Corner, and the Cost of a Path With Many Vertices

> A path says where; a trajectory says when, and the when has to be one that every motor and every gearbox can follow.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 30-05, 14-06, 29-08

## The Problem

A path is a list of configurations. An arm cannot be at a list of configurations; it has to move through them, in time, and the when matters to every part of it. A motor has a top speed and a top acceleration (29-08). A gearbox and the structure it drives do not like jerk, the rate at which acceleration changes: a profile whose acceleration jumps from zero to its limit in no time excites every resonance of the arm and is the sound of a robot that is hurting. Time parametrisation gives the path a clock that every joint can follow.

## The Concept

### An S curve for each segment

Along a straight segment of the path in joint space, the arm moves a distance `s`. The profile that moves it from rest to rest within a speed, an acceleration and a jerk limit is the S curve, seven phases: the jerk goes up to its limit so the acceleration ramps up; the acceleration is held at its limit; the jerk goes down, bringing the acceleration back to zero as the speed reaches its cruise; the speed cruises; and the whole thing is mirrored to slow down. All three are limited because the profile never changes acceleration faster than the jerk limit.

For a short segment the cruise speed is never reached, and for a shorter one even the acceleration limit is not: the peak speed is found in closed form in two regimes. If the acceleration reaches its limit `A`, then `distance = P (P / A + A / J)`, a quadratic in the peak `P`; if not, the acceleration is a triangle and `distance = 2 P to the power 1.5 over the square root of J`. A segment of 0.02 rad never reached the acceleration limit and a segment of 3 rad cruised at the full speed. Each of them ended exactly at the distance, at rest, within all three limits, which the test checks by sampling.

### Joint limits become limits along the segment

A segment has a direction `u` in joint space, with unit length. The distance `s` along it gives joint `i` a speed `u_i` times the speed along it, and the same for acceleration and jerk. So the limit along the segment is the smallest, over the joints, of the joint's limit over its share of the direction: a segment that is mostly joint 1 is limited by joint 1, a mixed one by whichever is nearest its limit. Over a path of 6 segments with different limits at each joint, no joint's speed, acceleration or jerk passed its limit (the largest shares of them were 0.727, 1.000 and 1.000, so the limits are used and not merely respected). See `E-ARM-0067`.

### What the jerk limit costs

A move of 1 rad with limits of 2 rad/s, 4 rad/s2 and 20 rad/s3 took 1.220 s with the S curve and 1.000 s with the trapezoid (which has no jerk limit): a little longer, by about the time it takes the acceleration to ramp, and that is the price. What it buys: the trapezoid's acceleration steps from zero to the limit in no time, so over a millisecond its jerk is 4000 rad/s3, against the limit of 20. See `E-ARM-0068`.

### A stop at every vertex

The segments are done one after the other, at rest at every vertex: the direction changes there, and a change of direction at speed is a jump in joint speeds, which no motor can do. So the number of vertices is part of the time: a wandering path of 17 points took 14.33 s, and its straight shortcut 1.45 s. This is the real reason for shortcutting (30-05): not elegance, seconds. Passing through a vertex without stopping needs the corner rounded by a curve the arm can follow, which is more than this lesson does. See `E-ARM-0069`.

### A speed override

Scaling the speed limits is how an operator slows an arm down. Halving them took a long move from 3.70 s to 6.45 s and left a short one unchanged, because a short move never reaches its cruise speed and is limited by acceleration and jerk.

## Build It

`scurve_peak` in `exercise/solution.hpp`.

```
rcpp verify 30-06
```

## Use It

`rc/arm/time_param.hpp`.

```cpp
rc::arm::JointLimits lim;  // per joint speed, acceleration and jerk
const auto traj = rc::arm::plan_trajectory(path, lim);
rc::arm::sample_trajectory(traj, t, q, qd, qdd, qddd);   // at each control tick
```

**What this does not do.** It stops at every vertex, so a path with a great many of them is slow; blending through corners within the limits is what a production planner does and is not here. The limits along a segment are the smallest over the joints, which is conservative for the others (it is not the time optimal trajectory). The joint limits are constants and independent of the pose: the torque limit of 29-08, which depends on the pose and the payload, is not a limit here, and a profile within a jerk limit can still ask for a torque that the motors cannot give (the check of 29-08 is a separate one). The path is straight in joint space between vertices, so the tool does not move in a straight line. And the S curve is a seven phase profile; some drives want a smoother one (a polynomial of higher order).

## What Breaks First

- **A path with no clock.** See `E-ARM-0067`.
- **A profile with a step in its acceleration.** See `E-ARM-0068`.
- **A path with a vertex for every little wiggle.** See `E-ARM-0069`.

## Ship It

`rc/arm/time_param.hpp` holds `JointLimits`, `SCurve`, `scurve_peak`, `scurve_plan`, `scurve_at`, `trapezoid_time`, `TrajSeg`, `plan_trajectory`, `trajectory_duration` and `sample_trajectory`.
