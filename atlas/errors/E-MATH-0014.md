id: E-MATH-0014
title: A smooth curve that loops when the waypoints are unevenly spaced
match: with uneven spacing the uniform curve loops, and the centripetal one does not
match: expected this to be true: turn_uniform
platforms: linux, windows
teaches: 06-09-a-path-through-the-waypoints
---

## Symptom

A route that was planned by hand, with a long run, a short step and a long run, comes out with a small loop or a cusp at the short step. The same curve through evenly spaced waypoints is fine.

## Cause

The simplest curve gives every waypoint the same time, however far apart. A short step then has to be crossed in the same time as a long run, and the curve has to move fast there and turn hard to fit. The timing parameter `alpha` fixes this: with the time growing as a power of the distance, a short step takes a short time.

Measured, waypoints with spacing 10, 1 and 14.1: the sharpest turn in one step was **178.2** degrees with `alpha` 0 (uniform, the curve turns back on itself), **9.1** with 0.5 (centripetal) and **8.7** with 1.0 (chordal).

## Fix

Time the curve by distance. The centripetal choice is a good default:

```cpp
const double t1 = t0 + std::pow(length(p1 - p0), alpha);      // alpha = 0.5
```
