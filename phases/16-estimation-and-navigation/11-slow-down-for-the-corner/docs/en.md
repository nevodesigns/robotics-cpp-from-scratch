# Slow Down for the Corner: Speed From Curvature, and Braking Before You Get There

> The planner knows where the corner is. The pursuit controller knows how to turn. Neither knows how fast the wheels can take it.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 16-04, 16-10

## The Problem

A path says where to go, and says nothing of how fast. A rover that drives it at its top speed, 2 m/s here, meets a corner of radius one metre at four times the sideways acceleration the wheels can hold: the tyres let go, or the load tips, or the odometry of lesson 16-01 loses the thread.
A rover that drives it slowly everywhere is safe and takes for ever.

The answer is a speed for each point of the path: fast where it is straight, slow where it bends, and arranged so that getting from one to the other does not ask the wheels for more than they have.

## The Concept

### Curvature from three points

A circle goes through any three points, and the curvature is one over its radius. It comes from the triangle they make, as twice its area over the product of its three sides:

```cpp
2.0 * std::fabs(cross) / (ab * bc * ca)
```

Points on a circle of radius r gave 1 / r for r of 0.5, 1, 2 and 10; points in a line gave zero; two coinciding gave zero rather than a division by it. See `E-NAV-0029`.

### A speed limit

Going round a curve at speed v needs a sideways acceleration of v squared times the curvature. Allow at most `max_lateral` and the limit is the square root of that over the curvature, never more than the top speed. With a limit of 1 m/s squared:

| corner radius (m) | speed limit (m/s) | predicted |
|---|---|---|
| 0.5 | 0.707 | 0.707 |
| 1.0 | 1.000 | 1.000 |
| 2.0 | 1.414 | 1.414 |
| 4.0 | 2.000 | 2.000 |
| 8.0 | 2.000 | 2.000 |

A corner of radius 4 or more is no limit at all at 2 m/s. At a steady 2 m/s the radius 1 corner gave 4.00 m/s squared sideways, and with the limits 1.00, which is what the wheels grip. See `E-NAV-0030`.

### Limits are not a plan

The limits say how fast each point may be taken, and give no thought to how the rover gets there. A drop from 2 to 1 m/s between neighbouring points, 5 cm apart, is a braking of nearly 20 m/s squared, and a jump back up is the same in the other direction:

| | worst gain (m/s^2) | worst loss (m/s^2) |
|---|---|---|
| limits alone | 19.81 | 19.81 |
| profile | 1.00 | 1.50 |

The **profile** respects the limits and the wheels. A forward pass from the start allows a speed no greater than the previous point's plus what acceleration can add over the distance, v squared = u squared + 2 a d. A backward pass from the stop, which is where the path ends, does the same with the braking. Where the limit drops, the backward pass
makes the rover start slowing early, in time. The profile starts and ends at rest, and under every limit. See `E-NAV-0031`.

### What grip costs

A corner of radius 1 m, with 20 m of straight on either side, a gain of 1.0 and a loss of 1.5:

| speed limit on the corner (m/s) | time (s) |
|---|---|
| 2.00 | 12.45 |
| 1.41 | 12.90 |
| 1.00 | 13.60 |
| 0.71 | 14.49 |

Halving the allowed sideways acceleration to 0.5 cost two seconds, or 16 percent of the run. That is the number to set against the grip. See `E-NAV-0031`.

## Build It

`curvature_at`, `speed_limits`, `speed_profile` and `travel_time` in `exercise/solution.hpp`. `distance_between` is given.

```
rcpp verify 16-11
```

The routes are made from exact arcs and straight lines, so the figures above are the same on every run.

## Use It

`rc/nav/speed.hpp` joins `rc::nav`.

```cpp
const std::vector<double> limits = speed_limits(path, 2.0, 1.0);
const std::vector<double> speeds = speed_profile(path, limits, 1.0, 1.5, current_speed);
```

**What this does not do.** It treats the three points as the curve, so a sharp corner in a path made of few points has a curvature that depends on how far apart the points are (and a vertex where two straight lines meet has an enormous one): smooth or resample the path first, or the limit at a corner is an
accident of sampling. It limits sideways acceleration only, and not the wheel speeds a differential drive needs on the outer wheel (lesson 16-04), nor the slope, nor a load on the rover. It plans a profile once for the whole path, and does not change it if the rover is later than the plan.

## What Breaks First

- **A curvature that divides by a zero side.** Coinciding points give a NaN. See `E-NAV-0029`.
- **The top speed everywhere.** The corner throws the rover sideways at four times the grip. See `E-NAV-0030`.
- **Limits used as speeds.** A stop asked for in one step. See `E-NAV-0031`.

## Ship It

`rc/nav/speed.hpp` holds `curvature_at`, `speed_limits`, `distance_between`, `speed_profile` and `travel_time`.

The rover now knows how fast to take every point of its route.
