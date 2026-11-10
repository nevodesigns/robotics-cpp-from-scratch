# A Path Through the Waypoints: Corners, Smooth Curves, and the Loop That Uneven Spacing Makes

> A planner hands over four waypoints. Joining them with straight lines gives a path with a corner at each, which
> nothing that moves can follow, and joining them with a smooth curve has a trap in how the curve is timed.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 06-01, 06-05

## The Problem

A robot is given waypoints and has to drive through them. Straight lines between them are the obvious path, and at every waypoint the direction changes by a whole angle in one step. A machine that has to turn on the spot at each waypoint is slow; one that tries not to overshoots.

A smooth curve through the waypoints has no corner. There are several, and the usual one has a setting, `alpha`, that decides how it is timed along the way. The wrong value makes a loop where the waypoints are unevenly spaced.

## The Concept

### A corner is a jump in direction

`max_turn_degrees` finds the sharpest change of direction between one step of a path and the next: 0 for a straight path, 90 for a square corner, 180 for turning back on itself. A square route of four waypoints, as straight lines, turned **90.0** degrees at a corner in one step.

The curve through the same waypoints, 20 points to a segment, turned at most **10.9** degrees in one of its steps, and was 30.93 long against 30.00. See `E-MATH-0013`.

### A Catmull-Rom curve

`catmull_rom_point(p0, p1, p2, p3, u, alpha)` gives a point between `p1` and `p2`, with `u` from 0 at `p1` to 1 at `p2`. The neighbours `p0` and `p3` decide the direction the curve arrives and leaves in, so it has no corner. It is built from three rounds of blending between neighbouring points, weighted by times: `t0 = 0`, then each next time is the last plus the distance to the next point, to the power `alpha`.

The curve passes through every waypoint, whatever `alpha` is. The test checks `u = 0` gives `p1` and `u = 1` gives `p2` for `alpha` 0, 0.5 and 1.

### The timing

| `alpha` | name | time to cross a segment |
|---|---|---|
| 0 | uniform | the same for every segment |
| 0.5 | centripetal | the square root of its length |
| 1 | chordal | its length |

Waypoints with spacing 10, 1 and 14.1: a long run east, a short step north, and a long run back to the north west.

| `alpha` | sharpest turn in one step | length |
|---|---|---|
| 0.0 | **178.2** degrees | 27.10 |
| 0.5 | **9.1** degrees | 25.32 |
| 1.0 | **8.7** degrees | 26.41 |

Uniform timing has to cross the short step as slowly as the long runs, so the curve rushes through it and turns back on itself. See `E-MATH-0014`.

### The ends and the repeats

The curve needs a neighbour before the first waypoint and after the last, which do not exist. Each is made by reflecting the next waypoint through the end, so the curve leaves and arrives the way the first and last segments point.

A waypoint that repeats gives a distance of zero, a time of zero, and a division by it. The sampler takes repeated waypoints once, and `catmull_rom_point` goes straight if any interval is under 1e-12. Waypoints on a line, with the last repeated, stayed exactly on the line for every `alpha`, with a sharpest turn of 0 and a length of **10**. See `E-MATH-0015`.

## Build It

`catmull_rom_point`, `path_length` and `max_turn_degrees` in `exercise/solution.hpp`. The point type, `sample_path` and the operators are given.

```
rcpp verify 06-09
```

## Use It

`rc/math/spline.hpp` joins `rc::math`. Its point type there is `Waypoint`.

```cpp
const std::vector<Waypoint> path = sample_path(waypoints, 20, 0.5);
if (max_turn_degrees(path) > limit) { /* slow down, or ask for more waypoints */ }
```

**What this does not do.** The curve is smooth in direction and says nothing about speed or acceleration: following it still needs a velocity profile (lesson 14-06). It may cut the inside of a corner or swing wide of it, and nothing stops it passing through an obstacle between two waypoints, so a path
has to be checked against the map afterwards. The curve is in the plane. And it goes through every waypoint exactly, which is wrong when the waypoints are noisy and the robot only needs to pass near them.

## What Breaks First

- **Straight lines between waypoints.** A corner at each. See `E-MATH-0013`.
- **Uniform timing on uneven waypoints.** A loop. See `E-MATH-0014`.
- **A repeated waypoint.** A division by zero. See `E-MATH-0015`.

## Ship It

`rc/math/spline.hpp` holds `catmull_rom_point`, `sample_path`, `path_length` and `max_turn_degrees`.

Lesson 06-05 took a derivative of a signal. This gives a path a direction that changes smoothly, which is what makes the derivative of the path worth taking.
