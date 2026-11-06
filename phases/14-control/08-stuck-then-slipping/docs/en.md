# Stuck, Then Slipping: Friction and the Limit Cycle

> A joint that sticks and an integral that keeps pushing do not settle. They hunt, for
> ever, by a small amount, and the cure for it has a price you can measure.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 14-04

## The Problem

A joint is told to hold a position. It creeps up towards it, stops short, then jumps past
it, stops on the far side, and does it again, slowly, for as long as the program runs. The
swing is small and the period is seconds, and it gets worse when the integral gain is
raised, the way a person raises it to close a steady error.

The same gains settle on a joint with no friction, so the tuning is not at fault. Friction
that sticks is a different kind of plant, and a loop with an integral on it has a way to go
wrong that the earlier lessons of this phase did not meet.

## The Concept

### Three kinds of friction, and the middle one is the trouble

**Viscous** friction pushes back in proportion to speed. It is gentle and linear, and
controllers are designed against it.

**Coulomb** friction pushes back with a fixed force whenever the mass is moving.

**Static** friction, stiction, holds a mass that is standing still with a force up to a limit,
and the limit is bigger than the Coulomb force. When the applied force passes it, the mass
lets go at once. A gearbox, a seal, a linear guide, a brake pad: all of them stick.

The consequence is a range of forces that will keep a moving mass going and will not start
it. For a 1 kg mass with a breakaway of 1.5 N and a Coulomb force of 1.0 N, 1.2 N from rest
moves it nowhere at all. 1.2 N applied once it is moving keeps it going at the speed where
the force less Coulomb balances the drag, `(1.2 - 1.0) / 0.5 = 0.4` metres a second.

### Friction has to be a state

The obvious way to write it is `sign(velocity) * coulomb`. At exactly zero velocity that is
zero, and in discrete time a mass crossing zero never lands on it. It is slightly positive,
so friction pushes it back, past zero, slightly negative, so friction pushes it back again.
It chatters across zero at the step rate for ever, and the force that should hold it is never
applied. See `E-CTRL-0019`.

So it is a state, stuck or slipping.

- **Stuck**: the mass does not move while the applied force is no more than the breakaway.
  Past it, it slips, and from then friction is the smaller Coulomb force.
- **Slipping**: friction opposes the motion, and if one step would carry the velocity through
  zero, the mass stops there and is stuck again.

The test checks it against the equations. A mass at 2 m/s with no force on it should stop
when `(m / b) ln(1 + b v / Coulomb) = 2 ln 2 = 1.386` seconds have passed, and the simulation
agrees to within a hundredth of a second. After that the velocity is exactly zero for five
thousand steps, not small.

### A proportional loop sticks short, and an integral hunts

A 1 kg mass with a breakaway of 1.5 N and a Coulomb friction of 1.0 N, held at one metre,
proportional gain 20 and damping 2, for sixty seconds with the last twenty judged:

| controller | final error | peak to peak | crossings | |
|---|---|---|---|---|
| no integral | 0.0623 | 0.0000 | 0 | settled, short |
| integral gain 5 | 0.0138 | 0.0000 | 0 | settled |
| integral gain 20 | -0.0196 | 0.0397 | 3 | **hunting** |
| integral gain 50 | -0.0249 | 0.0499 | 8 | **hunting** |

Without an integral the force stops growing once the error is small, so the mass sticks
somewhere inside a band: the breakaway over the gain, 1.5 / 20 = 0.075. It stopped at 0.0623
and never closes.

A small integral closes it and settles. A larger one winds up until the force passes the
breakaway. The mass lets go with more force than it needs to keep moving, goes through the
target, and the error changes sign. The integral begins to unwind. The mass slows, sticks on
the far side, and it happens again the other way. That is a limit cycle, and nothing in the
controller is wrong: it is the controller and the friction together. The harder the integral
the bigger and faster the swing. See `E-CTRL-0018`.

`analyze_hunting` says when a loop is doing it. It takes the error over a stretch of time and
reports the peak to peak swing and the number of crossings of the signal's own mean. Hunting
needs both: a swing big enough to matter, and crossings, so that a steady error, however
large, is not hunting and a swing too small to matter is not either.

### The cheap cure, and what it costs

Stop integrating when the error is small. Inside the band the loop is proportional only, so
the mass sticks, and stays.

The same loop with an integral gain of 20, the integral frozen while the error is smaller than
a band:

| band | final error | peak to peak | |
|---|---|---|---|
| none | -0.0196 | 0.0397 | hunting |
| 0.01 | -0.0051 | 0.0000 | settled |
| 0.03 | -0.0071 | 0.0000 | settled |
| 0.08 | -0.0367 | 0.0000 | settled, further off than the hunting went |

A narrow band, near what the joint can resolve, stops the hunting at a fraction of the error
the hunting had. A wide one is quiet and wrong by more than the oscillation ever was. The
band trades the hunting for a steady error about as large as the band. See `E-CTRL-0020`.

## Build It

`FrictionPlant::step` and `analyze_hunting` in `exercise/solution.hpp`. The comments say what
the two states do. The loops in the tests run on your plant, so a plant that chatters gives
numbers that mean nothing and the stopping test says so first.

```
rcpp verify 14-08
```

## Use It

`rc/control/friction.hpp` joins `rc::control`.

```cpp
rc::control::FrictionPlant joint;
// each tick
joint.step(force, 0.001);

const rc::control::HuntingReport r = rc::control::analyze_hunting(last_errors, 0.02, 2);
if (r.hunting) log_warning("joint is hunting");
```

**What this does not do.** It is one model of one kind of friction. Real friction is
dependent on speed (the Stribeck curve: it falls from the breakaway to the Coulomb level over
a range of low speeds), on position (a worn spot in a gearbox), on temperature, and on how
long the mass has been stationary. A joint with a gearbox also has backlash, a gap that the
controller crosses without effect, which produces its own limit cycle and which this
lesson does not model. The right band for a real joint is measured on it: command a slow ramp
and see where it lets go.

## What Breaks First

- **An integral on a joint that sticks.** It winds past the breakaway and hunts. See
  `E-CTRL-0018`.
- **Friction as sign times force.** The mass chatters across zero and never stops. See
  `E-CTRL-0019`.
- **A band wide enough to stop the hunting.** It costs accuracy by about its own width. See
  `E-CTRL-0020`.

## Ship It

`rc/control/friction.hpp` holds `FrictionPlant`, `HuntingReport` and `analyze_hunting`.

Lesson 14-07 found a ceiling on the gain from lateness. This finds a way a loop with an
integral can fail even when it is stable, and the shape it has, which is a small steady swing.
