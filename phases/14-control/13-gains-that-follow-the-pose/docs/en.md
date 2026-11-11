# Gains That Follow the Pose: One Controller for a Joint Whose Inertia Changes

> The gains were tuned with the arm folded, in the corner of the bench. The arm has not been folded since.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 14-04, 13-07

## The Problem

Lesson 13-07 showed what it takes to move an arm: the torque a joint needs depends on the pose, and so does the inertia it has to accelerate. A controller has one pair of gains, and the arm has many poses.

Gains chosen where the arm was easy to test, folded in, are right there and wrong everywhere else. Extended, the same torque moves a far greater inertia, the joint is slower and less damped, and the loop that was crisp becomes a slow ring.

## The Concept

### What the gains should be

For a joint of inertia J under a PD law, the closed loop has a natural frequency of the square root of kp / J and a damping ratio of kd over twice the square root of kp J. Turn that round and ask for a frequency w and a damping ratio zeta:

```cpp
kp = inertia * wn * wn;
kd = 2.0 * zeta * inertia * wn;
```

A joint of inertia 2 at w of 8 and zeta of 0.9 gets kp of 128 and kd of 28.8. Everything that varies with the pose is in the inertia. See `E-CTRL-0033`.

### Fixed gains, many poses

The test arm reaches out by an extension from 0 to 1, and the inertia about the joint is 0.5 + 2.5 e squared: the mass moves further out, and the inertia goes with the square of the distance. Gains designed folded, with w of 8 and zeta of 0.6, were kp 32 and kd 4.8:

| extension | inertia | overshoot | settling (s) |
|---|---|---|---|
| 0.00 | 0.50 | 0.087 | 0.74 |
| 0.25 | 0.66 | 0.136 | 0.83 |
| 0.50 | 1.12 | 0.245 | 1.57 |
| 0.75 | 1.91 | 0.354 | 2.73 |
| 1.00 | 3.00 | 0.444 | 4.32 |

Five times the overshoot and nearly six times the settling time, from nothing but reaching out. See `E-CTRL-0033`.

### Matched gains

Give each pose the gains for its own inertia:

| extension | kp | kd | overshoot | settling (s) |
|---|---|---|---|---|
| 0.00 | 32.0 | 4.8 | 0.087 | 0.74 |
| 0.25 | 42.0 | 6.3 | 0.088 | 0.74 |
| 0.50 | 72.0 | 10.8 | 0.091 | 0.74 |
| 0.75 | 122.0 | 18.3 | 0.092 | 0.74 |
| 1.00 | 192.0 | 28.8 | 0.093 | 0.74 |

One response at every pose. The gains went up sixfold to buy it. That is **gain scheduling**.

### A table, not a formula

On a real arm the inertia is not a tidy formula, and the controller does not work it out each cycle. It keeps a table of gains made at a few poses and joins them with lines. `lookup` does the joining:

```cpp
ys[i - 1] + (ys[i] - ys[i - 1]) * (x - xs[i - 1]) / (xs[i] - xs[i - 1])
```

and holds the end values outside the table, so a pose beyond the last knot gets the last knot's gains rather than a guess. See `E-CTRL-0034`.

### How many knots

The inertia is curved and the lines are straight, so between knots the gains are a little off. Settling time over 21 poses, fastest to slowest:

| knots | fastest to slowest (s) | spread (s) |
|---|---|---|
| 2 | 0.35 to 0.74 | 0.394 |
| 3 | 0.67 to 0.74 | 0.075 |
| 5 | 0.72 to 0.74 | 0.022 |

Two knots left the middle of the range nearly twice as fast as the ends, which is its own kind of wrong. Five were within 0.02 s. Put knots where the table is most curved. See `E-CTRL-0035`.

## Build It

`lookup`, `scheduled_gains`, `run_joint` and the `GainSchedule` constructor and `at` in `exercise/solution.hpp`. The inertia model is given.

```
rcpp verify 14-13
```

Everything is deterministic, so the figures above are the same on every run.

## Use It

`rc/control/schedule.hpp` joins `rc::control`.

```cpp
const GainSchedule schedule({0.0, 0.25, 0.5, 0.75, 1.0}, 8.0, 0.6);
// every control step:
const Gains g = schedule.at(extension);
const double torque = g.kp * (target - position) - g.kd * velocity;
```

**What this does not do.** It schedules on one number, the reach, for one joint. A real arm's inertia about a joint depends on every other joint, and a multi joint arm's schedule is a table over several numbers or, better, a model (lesson 13-07).
It does nothing about gravity or friction, which change with the pose too (lesson 14-05, lesson 14-08), and gains that rise with the inertia also rise with the sensor noise they amplify (lesson 14-11).
Check the rate the loop runs at as well, because the stiff end of the table is the one that needs it (lesson 14-12).

## What Breaks First

- **Gains from the corner of the bench.** Right folded, a slow ring extended. See `E-CTRL-0033`.
- **A lookup that extrapolates.** A pose outside the table gets gains no one designed. See `E-CTRL-0034`.
- **A table with too few knots.** The middle of the range is off. See `E-CTRL-0035`.

## Ship It

`rc/control/schedule.hpp` holds `Gains`, `JointResult`, `arm_inertia`, `lookup`, `scheduled_gains`, `run_joint` and `GainSchedule`.

Lesson 14-14 closes the phase with the opposite problem: gains for a machine nobody has a model of, found by experiment.
