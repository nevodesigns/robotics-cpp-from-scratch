# The Loop That Only Reacts Is Always Behind: Feedforward

> The larger half of the lag was not the plant resisting. It was the controller
> opposing its own motion.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 14-04, 06-06

## The Problem

The loop from phase 14 settles beautifully on a stationary target. Give it a
moving one and it lags behind by a fixed distance, for as long as the motion
lasts.

Not a transient. Not something that decays. A constant error, and every arm that
cuts a corner and every drive that runs behind its path has this at the bottom of
it.

## The Concept

### Why a reacting loop must be wrong

A feedback loop turns error into command. That is the whole of it. So if the
command has to be non-zero to keep the target followed, the error has to be
non-zero to produce it.

Measured on a 1 kg mass with damping 0.6 and a load of 0.4 N, following a ramp
at 0.5 m/s with kp = 20 and kd = 8: **0.234 m**, and it stays there.

Two causes, and they add:

**The plant needs force to move.** Holding 0.5 m/s takes `0.4 + 0.6 * 0.5 = 0.7 N`,
so the proportional term holds `0.7 / 20 = 0.035 m` of error to produce it.

**The derivative term opposes steady motion.** Taken on the measurement, it
contributes `-kd * v = -4 N` whenever the measurement is moving, and the
proportional term has to overcome that as well. Another `4 / 20 = 0.200 m`, and
it is the larger of the two.

The lesson's own test predicts that number from the formula and then measures it
in a simulation: 0.2350 against 0.2340.

### Two causes, two fixes, and neither does the other's job

| | steady error | worst after 2 s |
|---|---|---|
| feedback only, derivative on measurement | 0.234000 | 0.234001 |
| with feedforward for the plant | **0.199000** | 0.199001 |
| derivative on error instead | **0.034000** | 0.035042 |
| feedforward and derivative on error | 0.001000 | 0.001000 |
| integral instead of feedforward, ki = 20 | 0.001000 | **0.083422** |

Read the middle two rows against the decomposition. Feedforward removes 0.035,
which is exactly the plant's share. Moving the derivative removes 0.200, which is
exactly the controller's. Each removes its own and neither touches the other.

**Feedforward** is the command that comes from knowing what the target is doing
rather than from being wrong about it:

```cpp
double force_for(double velocity, double acceleration) const {
  return load + damping * velocity + mass * acceleration;
}
```

Three terms, three pieces of physics: the load is always there, the damping grows
with speed, the mass matters only while the speed is changing.

It needs a setpoint that carries a velocity and an acceleration. A setpoint that
is only a position has thrown both away, and anything generating a trajectory
already knows all three.

### The other derivative, and what it costs

The controller's share can also be removed by taking the derivative on the error
rather than the measurement. That is the last row's `0.034`, and it is not free.

A step of one metre, at 2 ms:

| derivative taken on | peak command |
|---|---|
| the measurement | 20.4 N |
| the error | **4020.4 N** |

Two hundred times the command, from the same gain, because the error's derivative
was five hundred metres a second for one sample. A measurement cannot jump; an
error can, whenever somebody moves the setpoint.

Neither is wrong. They are right about different inputs. So the combination to
reach for is **feedforward with the derivative left on the measurement**: no
spike, and the lag it leaves is the derivative's share alone. Where that still
matters, ramp the setpoint instead of stepping it, and then the two agree.

Whichever you choose, write it down next to the gains. It changes the meaning of
kd, so a gain copied between two loops that differ in this is not the same gain.

### Why not an integrator

An integrator does remove the error. Look at the last row again: it reaches the
same 0.001, and its worst excursion after two seconds is **0.083** against
feedforward's 0.001.

It gets there by building the force out of accumulated error, so it lags, then
overshoots, then settles. Feedforward supplies the force immediately because it
did not have to discover it.

An integrator is for the part of the load you did not model: drift, temperature,
an unknown payload. Using it for the part you did model means it is always
winding.

### A rough model beats no model

The objection to feedforward is always that the model is not known. Measured,
with the damping term estimated wrongly:

| guess / true | steady error |
|---|---|
| 0.0 | 0.014000 |
| 0.5 | 0.006500 |
| 1.0 | 0.001000 |
| 1.5 | 0.008500 |
| 2.0 | 0.016000 |

With no feedforward at all: **0.234000**.

Leaving the damping term out entirely, and feeding forward only the load and the
inertia, was still sixteen times better than nothing. The penalty is proportional
to how wrong the model is, symmetric, and there is no cliff.

And the model is measurable in an afternoon: one run at constant speed gives the
load and damping together, two runs at two speeds separate them, one run with a
known acceleration gives the inertia.

### Limit once

```cpp
const double total = feedback + model.force_for(target.velocity, target.acceleration);
return clamp(total, lowest, highest);
```

Clamping the feedback and then adding to it gives a total that exceeds the limit.
Clamping both separately throws away authority the actuator has.

And a feedforward that reaches the limit on its own is worth reporting rather
than hiding: the profile is asking for more than the machine has, before any
error has happened, and the answer is to slow the profile rather than let the
loop discover it as a tracking failure.

## Build It

Implement `force_for`, `ramp_error` and `command` in `exercise/solution.hpp`.

```
rcpp verify 14-05
```

The suite predicts the standing error from the formula and then measures it,
separates the two causes by removing each in turn, steps the setpoint to see what
each derivative costs, and sweeps a deliberately wrong model.

## Use It

**Carry velocity and acceleration in the setpoint**, everywhere. It costs two
doubles and it is the only thing feedforward needs.

**Feed forward what you can model and leave the integrator for the rest.**

**Limit the total, once.**

**Say which derivative the loop uses**, beside the gains.

**Measure the model.** The table above is what an afternoon with a stopwatch
buys, and it beats any amount of gain tuning at removing a lag that a higher gain
can only divide.

## What Breaks First

- **A constant error that never decays.** See `E-CTRL-0009`.
- **A derivative that fights motion, or explodes on a step.** See `E-CTRL-0010`.
- **A model treated as though it had to be right.** See `E-CTRL-0011`.

## Ship It

`PlantModel`, `Setpoint`, `ramp_error` and `command` join `rc::control` beside
the PID and the tuning tools. Every loop from here can be told what the target is
doing instead of working it out from being wrong, and the follower in 16-04 has
something to hand it.
