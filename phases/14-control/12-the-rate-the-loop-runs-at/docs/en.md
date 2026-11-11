# The Rate the Loop Runs At: How Slow Can the Controller Be Before It Falls Over

> A controller looks at the joint, decides, and then does not look again until its next tick. What the joint does in between is none of its business, and the gains were chosen as if it were.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 14-04, 14-07

## The Problem

Every gain in this phase so far was tuned as if the loop ran continuously. A real controller runs once every few milliseconds, reads the sensor, writes one number to the
motor, and the motor holds that number until the next tick.

Lesson 14-07 put a delay in the loop on purpose and showed how it eats the margin. A slow sample rate is a delay too: on average, half a period of staleness in everything the controller
knows and does. The question is how long a period the gains can stand, and the answer is a number you can measure rather than guess.

## The Concept

### A sampled loop

The test plant is a unit mass with a little damping. It moves in steps of 0.1 ms. The controller is the same position and speed law as before, ticking only every `period` seconds.

```cpp
if (i % per == 0) force = kp * (1.0 - position) - kd * velocity;
velocity += (force - damping * velocity) * h;
position += velocity * h;
```

Between ticks the force is a constant, the **zero order hold** a real motor driver gives.

### Slower means more overshoot

At kp 100 and kd 10 the same loop, only the period changed:

| period (s) | overshoot |
|---|---|
| 0.001 | 0.156 |
| 0.010 | 0.165 |
| 0.050 | 0.228 |
| 0.100 | 0.398 |
| 0.150 | 0.791 |

Nothing about the gains changed. The loop got worse only because the controller looked less often, and the damage grows faster than the period does. See `E-CTRL-0030`.

### And then it stops working

At 0.3 s the same gains were not stable, and neither were they at 2 s. There is a **largest stable period**, and finding it is a bisection: if the loop is stable at the midpoint, the answer is longer than that, otherwise shorter.

```
largest stable period at kp 100, kd 10: 0.1821 s
```

Stable at 97 percent of it, not stable at 105 percent. See `E-CTRL-0030`.

### Stiffer gains, faster loop

| kp | kd | largest period (s) | limit times natural frequency |
|---|---|---|---|
| 25 | 5 | 0.3424 | 1.71 |
| 100 | 10 | 0.1821 | 1.82 |
| 400 | 20 | 0.0950 | 1.90 |

Quadrupling kp, which doubles the natural frequency, roughly halved the limit. The product of the two stays near 1.8, so the limit is about 1.8 over the natural frequency, and a joint twice as stiff needs a loop twice as fast. See `E-CTRL-0031`.

### A share of the limit

A loop is not run at its limit. It is run at a share of it, and the share is the price of closeness to the continuous design:

| share of the limit | period (s) | overshoot | against 1 ms |
|---|---|---|---|
| 0.05 | 0.0091 | 0.164 | 1.05 |
| 0.10 | 0.0182 | 0.175 | 1.12 |
| 0.25 | 0.0455 | 0.219 | 1.40 |
| 0.50 | 0.0911 | 0.353 | 2.26 |

A tenth of the limit cost 12 percent more overshoot and a half cost more than double. A tenth to a twentieth is a common place to start. See `E-CTRL-0032`.

## Build It

`run_sampled`, `largest_stable_period` and `period_for` in `exercise/solution.hpp`.

```
rcpp verify 14-12
```

Nothing here uses a random number or the clock, so the figures above are the same on every run. The whole suite takes a fraction of a second.

## Use It

`rc/control/sample_rate.hpp` joins `rc::control`.

```cpp
const double limit = largest_stable_period(kp, kd);   // for this plant
const double period = period_for(kp, kd, 0.1);        // run at a tenth of it
```

**What this does not do.** It measures one plant, a mass with a little damping, and one control law. A plant with its own delay (lesson 14-07), friction (lesson 14-08) or a lag in its sensor
has a shorter limit, and the number above is only the start of a budget. It also assumes the period is steady: a loop that sometimes runs late, because something else held the CPU, needs
the late tick to be inside the limit too. The simulation steps at 0.1 ms, so a period below that is the same as 0.1 ms.

## What Breaks First

- **Gains tuned on a continuous model.** The overshoot grows as the period does. See `E-CTRL-0030`.
- **Gains raised, rate left alone.** The limit falls with the natural frequency. See `E-CTRL-0031`.
- **A rate chosen at the edge.** It works until a late tick. See `E-CTRL-0032`.

## Ship It

`rc/control/sample_rate.hpp` holds `LoopResult`, `run_sampled`, `largest_stable_period` and `period_for`.

Lesson 14-13 changes the plant instead of the clock: what to do when the same gains meet a different machine.
