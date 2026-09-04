# How Far Ahead to Look: The Lookahead Is a Gain

> With a perfect estimate there is no argument for a long lookahead at all.
> Nobody has a perfect estimate.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 16-03, 06-01

## The Problem

The robot knows roughly where it is. There is a path. Something has to turn the
second into wheel speeds.

Pure pursuit is the method almost everybody uses first, and deservedly: it is one
line of geometry. Pick the point on the path a lookahead distance ahead, and
drive the arc that reaches it.

```cpp
curvature = 2 * left / (forward * forward + left * left);
```

Which leaves exactly one number to choose, and it looks like a preference. It is
not. **The lookahead is a gain**, and what it is a gain on is the error in the
pose the follower is steering by.

## The Concept

### With a perfect estimate, shorter is simply better

A right angle corner at 1 m/s, steering by the truth:

| lookahead | rms | worst | worst / lookahead |
|---|---|---|---|
| 0.10 m | 0.0015 | 0.0176 | 0.176 |
| 0.20 m | 0.0044 | 0.0405 | 0.202 |
| 0.40 m | 0.0119 | 0.1047 | 0.262 |
| 0.80 m | 0.0326 | 0.2125 | 0.266 |
| 1.50 m | 0.0828 | 0.4009 | 0.267 |
| 3.00 m | 0.2355 | 0.8093 | 0.270 |
| 5.00 m | 0.4595 | 1.3536 | 0.271 |

No U, no oscillation, no minimum. Every longer lookahead is worse than the one
before it, and nothing in this table argues for anything but the shortest value
you can compute.

The last column is worth keeping on its own: **a right angle corner is cut by
about 27 percent of the lookahead**, and that ratio holds across two decades of
it. A 1 m lookahead means a quarter of a metre of corner, always, by design
rather than by failure.

### The estimate is what decides it

Now steer by what the robot believes rather than by what is true. Same corner,
same speed:

| lookahead | 2 cm of noise: rms | churn | 100 ms late: rms | churn |
|---|---|---|---|---|
| 0.10 | 0.0064 | **3.5824** | **lost** | 0.0895 |
| 0.20 | 0.0063 | 0.9849 | 0.0104 | 0.0941 |
| 0.40 | 0.0124 | 0.2625 | 0.0077 | 0.0082 |
| 0.80 | 0.0327 | 0.0754 | 0.0264 | 0.0034 |
| 1.50 | 0.0829 | 0.0292 | 0.0746 | 0.0017 |

Two different failures, and they are worth telling apart.

**Noise is paid for in the actuator.** With 2 cm of pose noise a lookahead of
0.10 m still tracked slightly better than 0.80 m did, and it moved the steering
about fifty times as much doing it. Nothing about the path looks wrong. The
motors, the gearbox and the current draw know.

**Latency is paid for in the path.** At 0.10 m of lookahead with a pose 100 ms
behind, the entry is not a large number, it is `lost`: the robot left the path
and did not come back.

The mechanism is the same in both cases. Aiming at a point a lookahead away
turns a pose error `e` into a steering command of about `2e / L²`, so halving the
lookahead quadruples what the same error does. When the error reaches the
lookahead, the target point is inside the error, and the command is noise.

### The rule that falls out

If the lookahead has to clear the pose error, and a late pose is wrong by speed
times its latency, then the smallest workable lookahead should be speed times
latency. Measured, with a pose 100 ms behind:

| speed | speed x 0.1 s | 0.05 | 0.10 | 0.20 | 0.40 |
|---|---|---|---|---|---|
| 0.5 m/s | 0.05 | lost | 0.0023 | 0.0028 | 0.0098 |
| 1.0 m/s | 0.10 | lost | lost | 0.0104 | 0.0077 |
| 2.0 m/s | 0.20 | lost | lost | lost | 0.0415 |
| 4.0 m/s | 0.40 | lost | lost | lost | lost |

Every lookahead at or below the distance travelled during the delay fails. The
first one above it works. At every speed, on the diagonal, exactly.

So the lookahead scales with speed:

```cpp
const double lookahead = std::max(min_lookahead, gain * speed);
```

where `gain` is the estimate's latency with margin and `min_lookahead` clears the
noise for standing starts.

**A fixed lookahead is a maximum speed nobody wrote down.** It is correct at the
speed somebody tuned it at, and the robot leaves the path when a later change
makes it faster.

There is a second reading of the same table. Every millisecond of latency you
remove from the estimate is lookahead you get to spend on cornering instead, so
phase 15's work on transport delay pays for itself again here.

### The search that must not look backwards

The other decision inside the method is which point counts as the one ahead, and
the natural implementation is wrong.

Written from the description, the loop is: find the first point at least a
lookahead away. From a metre along the path, the start of the path is also more
than a lookahead away, and it comes first in the array.

On a path that runs 8 m out, crosses over and returns along a leg 0.6 m away,
20 m in total:

| search | outcome |
|---|---|
| forward only, from the last target | arrived, 19.34 m driven |
| from the start of the path, every tick | **lost, 80.00 m driven, ending at (80.00, 0.00)** |

The second robot never turned once. A point directly behind it has no sideways
offset, so the curvature is zero, so it drove in a straight line until the test
gave up on it.

Keep the last target's index and never search before it. That makes the target
monotonic along the path however close two parts of the route come to each
other, and it is faster as well.

**A straight test path cannot tell the two versions apart.** Neither can a single
gentle curve. That is the general shape of this lesson: every failure in it was
invisible until the test contained the thing that provokes it.

## Build It

Implement `target_index`, `follow` and `cross_track_error` in
`exercise/solution.hpp`.

```
rcpp verify 16-04
```

The suite checks the geometry, then drives the corner seven ways with a perfect
pose, then again with a noisy one and a late one, then sweeps speed against
lookahead, then drives a path that comes back on itself.

## Use It

**Scale the lookahead with speed**, floor it above the noise, and write the
latency it assumes in the comment beside the gain.

**Give the planner the corner cut.** About 0.27 of the lookahead for a right
angle. Inflating obstacles by that is a clearance somebody can defend; a
tracking error figure is not, because a robot 20 cm inside a corner can report
3 cm of cross track error honestly.

**Measure to the path, not to the nearest waypoint.** Those differ by up to the
waypoint spacing, and the difference always flatters the result.

**Put a corner, a returning leg and a speed change in the acceptance path.** Each
one catches a different failure here, and a straight line catches none of them.

## What Breaks First

- **A lookahead shorter than the pose error.** See `E-NAV-0008`.
- **A corner cut nobody told the planner about.** See `E-NAV-0009`.
- **A target search that can look backwards.** See `E-NAV-0010`.

## Ship It

`PurePursuit` joins `rc::nav` beside the filter and the delayed measurement. The
estimate from 16-03 now has something to drive, and the two are connected by a
number: the follower's smallest safe lookahead is set by the estimator's
latency, so improving one directly buys the other.
