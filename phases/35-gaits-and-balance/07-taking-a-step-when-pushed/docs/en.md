# Taking a Step When Pushed: The Capture Point, Where a Foot Must Land, and How the Time to Get It There Limits the Push That Can Be Answered

> After a push the body is falling toward a point on the ground, and a foot that lands beyond that point catches it; the later the foot lands, the further the point has gone.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 35-06, 35-02, 34-05

## The Problem

35-05 said how hard a body can be pushed and still be held level by its feet: it is limited by the feet's geometry, and past that limit stiffer gains do not help. What helps is a step: put a foot down further out, to widen the support under the falling body. The questions are where, and whether there is time. The answer is a point of the inverted pendulum, the capture point.

## The Concept

### The capture point

A body a distance x from the foot (or the line of the feet) that is down, moving away at v, h high, is an inverted pendulum (35-02). The place on the ground over which it would come to rest is the capture point: `x plus v times the time constant T = sqrt(h/g)`. A foot put there (or beyond) catches the body; nearer and the body goes on over it. A body 0.27 m up has T = 0.166 s. A push of 0.30 m/s sideways, from rest over the line of the stance feet, put its capture point 5.0 cm out. See `E-LEG-0042`.

### It runs away

If nothing is done the capture point moves away exponentially: xi0 e to the t over T. In the time of one T it has grown 2.7 times. So a foot that lands after a swing time t must land at the capture point of the landing time, not of the push: with a swing of 0.25 s the foot had to land 24.5 cm out (the capture point then, plus a margin of 2 cm), against 5.0 cm at the push. See `E-LEG-0043`.

### Whether the leg can reach

The foot can only land within the leg's reach from the support (here 25 cm), and the target for this push was 24.5 cm: just within it (1, 1 for yes). Turning it round, the largest push that can be answered for a swing of 0.15 s was 0.56 m/s, for 0.25 s 0.31 m/s and for 0.35 s 0.17 m/s: each tenth of a second of swing costs a large share of the push that can be taken, because of the exponential. A fast swing (34-05's path is limited by the servos' rate) is worth much more here than a long leg. See `E-LEG-0044`.

### The result

A foot landed at the target left the capture point 2.0 cm inside it, the margin, so the body stops over the new foot; one landed short left the capture point outside and the body goes on falling. The step is the answer once the feet's moment (35-05) is spent: use the feet's forces for small pushes and a step for large ones.

## Build It

`step_target` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- The capture point at the landing time is xi0 e^(t_land / T) (capture_at); the target is that plus the margin; ok if the target is within the reach.

As shipped, 2 of the 3 tests fail, and these are the ones your code has to turn green:

- The foot must land beyond the capture point of the landing time, and the push that can be answered falls with the swing time.
- A foot landed at the target leaves the capture point inside it, and one landed short leaves it outside.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 35-07
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/legged/push_recovery.hpp`.

```cpp
const double xi = rc::legged::capture_point(x_from_support, v_away, com_height);
const rc::legged::StepPlan p = rc::legged::step_target(xi, swing_time, com_height, leg_reach, 0.02);
if (!p.ok) brace_for_the_fall();
```

**What this does not do.** It is one dimensional (the direction away from the support) and the pendulum is linear and rigid, with a constant height; the foot is taken to land exactly where told (34-05's path must get there in the swing time), and the ground there must be flat and free (35-08). The margin is a number. The leg's reach is a given distance from the support, not worked out from the leg's geometry (34-01). The time to decide (sensing the push, 34-07, 35-04) is not counted and eats into the swing time in a real robot. The numbers are typical of a 12 kg robot.

## What Breaks First

- **A step aimed at the capture point of the push.** See `E-LEG-0043`.
- **A foot put down at the body.** See `E-LEG-0042`.
- **A push response designed without the swing time.** See `E-LEG-0044`.

## Ship It

`rc/legged/push_recovery.hpp` holds `StepPlan`, `time_constant`, `capture_point`, `capture_at`, `step_target` and `max_push_speed`.
