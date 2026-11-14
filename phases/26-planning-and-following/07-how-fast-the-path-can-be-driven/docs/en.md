# How Fast the Path Can Be Driven: A Speed for Every Point, Within the Grip of a Turn, the Brakes Before It, and the Stop Line

> A speed limit is not a number for the vehicle, it is a number for every point of the path, and the points before it.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 26-03, 23-05

## The Problem

Lesson 26-04 drove a path at one speed, and 26-06 capped the speed in a turn by looking at the pose it was in. By then the vehicle is in the turn, going too fast to slow down in time. A speed has to be planned for the points ahead, from the path: the speed of the bend is also a limit on the speed before the bend.

## The Concept

### Three passes

For each point of the path, in this order:

1. **Cap it.** By the top speed, and by the speed at which the sideways acceleration `v squared times the curvature` reaches a limit `a_lat` (from the grip of the surface: 0.7 of mu g). A stop line, or any other ceiling, is a cap too (a speed of zero).
2. **Backward pass, from the end.** A point can be no faster than a speed from which the vehicle could still slow down to the point after it, at the braking limit: `v squared <= (next v) squared + 2 a_dec ds`. This is what puts the brakes on before the bend.
3. **Forward pass, from the start.** A point can be no faster than what the vehicle could have reached from the one before it at the acceleration limit: `v squared <= (previous v) squared + 2 a_acc ds`.

The result is a speed for every point which breaks no limit. On a path of a straight, a quarter circle of radius 1 m and a straight, the speeds never passed the sideways limit. The capped speeds alone (pass 1) asked for a deceleration of 20.0 m/s2 going into the bend, which no brake gives; the profile asked 1.50 against a limit of 1.5. See `E-VEH-0100`.

### A stop line

A cap of zero at a point is a stop line. The vehicle reached it at zero speed, with the slowing begun 1.30 m before it (at 1.5 m/s2, from 2 m/s the distance is v squared over 2a, 1.33 m). Without the backward pass the vehicle would reach the line at full speed and stop on the other side of it. See `E-VEH-0101`.

### The ground sets the pace

The same path took 5.9 s on tarmac and 8.0 s on ice, with nothing changed but the grip. A speed profile is where the surface knowledge of 22 and 23 pays off in time. A corner of 90 degrees with points 0.2 m apart took 8.6 s as it was, because the curvature at the corner point asks a crawl, and 8.2 s after smoothing (26-03). See `E-VEH-0102`.

## Build It

The three passes in `speed_profile`, in `exercise/solution.hpp`.

```
rcpp verify 26-07
```

## Use It

`rc/drive/speed_profile.hpp`.

```cpp
rc::drive::SpeedLimits lim;
lim.a_lat = rc::drive::lateral_limit(mu, 0.7);
const auto v = rc::drive::speed_profile(path, lim, &caps);
```

**What this does not do.** The curvature is read from three points of a path, which is noisy when the points are close and the path has been drawn by a planner: a single bad point asks for a crawl. The acceleration limits are constants; real motors give less at speed and on a slope (23-06). The sideways acceleration and the braking are limited separately; a vehicle braking in a turn shares one grip between both (the traction circle, 23-05) and this profile does not. The surface is one number for the whole path. The profile is for one path: if the path changes, so does the profile.

## What Breaks First

- **A speed limit that begins at the bend.** See `E-VEH-0100`.
- **A stop line met at speed.** See `E-VEH-0101`.
- **A corner that makes the whole run slow.** See `E-VEH-0102`.

## Ship It

`rc/drive/speed_profile.hpp` holds `SpeedLimits`, `lateral_limit`, `path_curvature`, `speed_profile`, `profile_times`, `max_decel` and `max_accel`.
