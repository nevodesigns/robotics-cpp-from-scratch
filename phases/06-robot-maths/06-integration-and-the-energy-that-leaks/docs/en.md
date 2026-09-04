# Integration: The Simulation That Gains Energy

> Over one second the explicit method was the more accurate of the two. Over ten
> minutes it was holding twenty six billion times the energy it started with.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 06-05, 00-05

## The Problem

Lesson 06-05 took a derivative. This one goes the other way: given how fast
something is changing, work out where it gets to.

Every simulation is this. The robot in phase 00 moves because a position was
advanced by a velocity; the plant in phase 14 responds because a force became an
acceleration became a velocity became a position. Two lines of code, written
without thinking about it, and one of them is wrong in a way that does not show
up for ten minutes.

## The Concept

### The obvious version manufactures energy

```cpp
next.position = position + velocity * dt;
next.velocity = velocity + acceleration(position) * dt;
```

Both halves advanced from where the step began. It is the obvious reading of the
definition and it is what almost everybody writes first.

A mass on a spring at 2 rad/s, stepped at 0.01 s, energy as a fraction of where
it started:

| seconds | explicit Euler | semi-implicit | RK4 |
|---|---|---|---|
| 100 | 54.5545 | 1.008696 | 1.000000 |
| 200 | 2976.1935 | 0.991266 | 1.000000 |
| 300 | 162364.7522 | 1.000684 | 1.000000 |
| 400 | 8857727.9716 | 1.008327 | 1.000000 |
| 500 | 483228926.0691 | 0.990967 | 1.000000 |
| 600 | **26362312744.2642** | 1.001365 | 1.000000 |

Ten simulated minutes, and a spring nobody pushed is holding twenty six billion
times the energy it was given.

The error is not symmetric. Every step adds a little, and the addition compounds.
Halving the step halves the rate of the gain and does not remove it, which is why
this always feels like something that just needs tuning.

### One line

```cpp
next.velocity = velocity + acceleration(position) * dt;
next.position = position + next.velocity * dt;
```

Advance the velocity first; advance the position with the velocity that came
out. Same two multiplications, same cost, same order of accuracy.

The energy stays between 0.991 and 1.009 across the whole run. It never
converges to the right value and **it never leaves**, which is a different and
much more useful guarantee than being accurate for a while.

This is semi-implicit Euler, also called symplectic Euler, and it is the method
under most game physics and most robot simulators. The reason is not accuracy.

### Accuracy and stability are different properties

Error in position after one second, at a step of 0.02:

| method | error |
|---|---|
| explicit Euler | 1.596e-02 |
| semi-implicit Euler | 1.831e-02 |

**Explicit Euler wins.** The method that gains twenty six billion times its
energy is the more accurate of the two over a one second test, which is the
length of test that fits comfortably in a unit suite.

Nor does the order of the method tell you. Both Euler methods are first order:

| dt | Euler | ratio | semi-impl | ratio | RK4 | ratio |
|---|---|---|---|---|---|---|
| 0.02000 | 1.596e-02 | | 1.831e-02 | | 3.937e-08 | |
| 0.01000 | 8.158e-03 | 1.96 | 9.124e-03 | 2.01 | 2.443e-09 | 16.11 |
| 0.00500 | 4.121e-03 | 1.98 | 4.554e-03 | 2.00 | 1.521e-10 | 16.06 |
| 0.00250 | 2.071e-03 | 1.99 | 2.275e-03 | 2.00 | 9.490e-12 | 16.03 |
| 0.00125 | 1.038e-03 | 2.00 | 1.137e-03 | 2.00 | 5.932e-13 | 16.00 |

Half the step, half the error, for both. Nothing in those columns separates the
method that conserves energy from the one that creates it.

So: **watch a conserved quantity, over the horizon you will actually run.**
Energy, momentum, the norm of a quaternion, the determinant of a rotation. A
quantity that should not change is a far better instrument than a trajectory
that should.

### Runge Kutta, and how to compare it fairly

Four evaluations of the acceleration, at the start, twice in the middle, and at
the end, weighted so that the errors of the first three orders cancel. The right
hand column above is fourth order to two decimal places.

The temptation is to say it costs four times as much. Compare at equal work
instead:

| | error after one second |
|---|---|
| Euler at 0.00500 s | 4.121e-03 |
| RK4 at 0.02000 s | **3.937e-08** |

The same four calls to the acceleration per simulated 0.02 s, and five orders of
magnitude between them. Comparing at equal *step* makes a cheap method look
competitive when it is not.

### The instrument has a resolution too

One more measurement, and it is a warning about all the others.

Runge Kutta run for one second with more and more steps, against the closed form
answer:

| steps | step size | error |
|---|---|---|
| 100 | 1.00e-02 | 2.443e-09 |
| 1 000 | 1.00e-03 | 2.425e-13 |
| **10 000** | 1.00e-04 | **3.886e-16** |
| 100 000 | 1.00e-05 | 1.116e-14 |
| 1 000 000 | 1.00e-06 | 7.161e-15 |
| 10 000 000 | 1.00e-07 | 3.392e-14 |

The best reference took ten thousand steps. A thousand times more of them was
about ninety times worse, because every step rounds and a million roundings are
larger than the truncation they were spent removing.

It is the same V as the derivative in lesson 06-05, from the same two errors
pulling opposite ways. Anywhere a smaller step is supposed to buy accuracy,
there is a floor and then a wall.

Which is why every number in this lesson is measured against a closed form. A
mass on a spring has an exact solution, so what these tables measure is the
method and nothing else. **You cannot measure an error smaller than your
reference's own.**

## Build It

Implement `euler_step`, `semi_implicit_step` and `rk4_step` in
`exercise/solution.hpp`.

```
rcpp verify 06-06
```

The suite takes one step of each by hand, runs all three for ten simulated
minutes watching the energy, sweeps the step to find each method's order,
compares them at equal work, and then measures how good a reference solution can
get before it starts getting worse.

## Use It

**Semi-implicit by default** for anything with inertia that runs for a long time.
It is free and it does not blow up.

**Runge Kutta where the trajectory itself has to be right**, and compare it
against the alternatives at equal work.

**Never explicit Euler for a second order system**, unless the run is short and
you have written down why.

**Watch a conserved quantity in the simulation's own tests.** One line, and it
catches this class of defect the first time it appears rather than in the run
that mattered.

**Test against a closed form.** A linear oscillator, a constant acceleration, a
body in free fall. They are not toy problems, they are instruments.

## What Breaks First

- **A simulation that gains energy.** See `E-NUM-0014`.
- **Accuracy measured, stability assumed.** See `E-NUM-0015`.
- **A reference built by taking more steps.** See `E-NUM-0016`.

## Ship It

`euler_step`, `semi_implicit_step` and `rk4_step` join `rc::math` as
`rc/math/integrate.hpp`. Every simulation from here has three integrators to
choose between and a measured reason for the choice, rather than the two lines
that happen to come to mind first.
