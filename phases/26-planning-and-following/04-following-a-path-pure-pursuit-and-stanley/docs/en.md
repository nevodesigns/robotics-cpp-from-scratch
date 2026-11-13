# Following a Path: Pure Pursuit and Stanley, Where Each Cuts the Curve or Swings Across the Line

> A follower chases a point on the path; how far ahead that point is decides everything.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 26-03, 23-03

## The Problem

A path is a list of points. A car needs, many times a second, **one number: the steering angle**. A follower turns the one into the other, and a bad one wins in any way that matters on the day: it cuts a corner into a wall, it swings across the line like a drunk, or it never settles.

## The Concept

### Pure pursuit: chase a point

Pick a point on the path a **lookahead** distance ahead. Steer along the arc of the bicycle model that reaches it: if the point is at an angle `alpha` from the heading and a distance `d`, the steering is `atan2(2 L sin(alpha), d)` (L is the wheelbase). That is the whole method. The only choice is the lookahead, and it decides everything.

Starting 1 m to the side of a straight path, pure pursuit with a lookahead of 1.0 m ended **0.000 m** off the path. A **short** lookahead turns hard to get back, overshoots, and turns hard again: with a lookahead of 0.1 m the car crossed the path **9 times**; with 1.5 m, **2**. A **long** lookahead is smooth on a straight and **cuts every curve**: where a straight turned into a circle of radius 3 m, a lookahead of 0.6 m strayed from the path by at most **0.015 m** and one of 2.0 m by **0.149 m**. (On a circle it is already on, pure pursuit is exact: the arc to a point of the circle is the circle. The cut comes where the curvature **changes**.) See `E-VEH-0091`.

### Stanley: correct what is wrong now

Stanley, the method of the car that won the 2005 desert race, uses no lookahead point. It steers by the **heading error** (the angle between the path's direction and the car's) plus an angle from the **cross-track error of the front axle**: `atan2(-k e, v)`, which turns the car back towards the path, more gently at speed. From the same 1 m offset it ended **0.000 m** off. At the same bend it strayed by at most **0.036 m**. It has a price: it follows the path exactly, kinks and all, and the heading error of a path with a corner is a step. See `E-VEH-0092`.

### What each does at a corner

On a corner of 90 degrees pure pursuit cut inside by **0.38 m** (the closest the car came to the corner point). On the smoothed path (lesson 26-03) it came within **0.41 m**, no closer: **smoothing the path did not stop the cut**, because the lookahead was doing the cutting, and a lookahead is a smoother of its own. The way to keep the car off the inside of a corner is clearance in the plan. See `E-VEH-0093`.

## Build It

`pure_pursuit_steer` and `stanley_steer` in `exercise/solution.hpp`.

```
rcpp verify 26-04
```

## Use It

`rc/drive/follow.hpp`.

```cpp
rc::drive::FollowConfig cfg;
cfg.method = rc::drive::kStanley;
const rc::drive::FollowRun run = rc::drive::follow_path(path, start, cfg);
```

**What this does not do.** The lookahead is fixed; vehicles that vary it with speed (a longer lookahead at speed, so the same number of seconds ahead) are the usual improvement and are the next step for a car that drives at more than one speed. The steering is instant: a real steering takes time to move, which adds the oscillation these tests do not show. The simulation is the kinematic bicycle model, with no slip and no delay. It does not stop at the end or control speed (26-07 does speed). The signed distance to the path is the distance to the nearest segment, which is wrong in the inside of a very tight bend. It has no obstacles.

## What Breaks First

- **A lookahead that is too short or too long.** See `E-VEH-0091`.
- **Stanley on a path with kinks.** See `E-VEH-0092`.
- **A corner cut into the wall.** See `E-VEH-0093`.

## Ship It

`rc/drive/follow.hpp` holds `FollowMethod`, `FollowConfig`, `FollowRun`, `clamp_steer`, `cross_track`, `nearest_index`, `pure_pursuit_steer`, `stanley_steer` and `follow_path`.
