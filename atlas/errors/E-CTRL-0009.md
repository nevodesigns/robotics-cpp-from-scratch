id: E-CTRL-0009
title: A constant error that never decays, while the target keeps moving
match: the error a reacting loop is left with, predicted and then measured
match: what the plant needs, before anything has gone wrong
platforms: linux, windows
teaches: 14-05-the-loop-that-only-reacts
---

## Symptom

A loop that settles perfectly on a stationary target lags behind a moving one by
a fixed distance, for as long as the motion lasts. It is not a transient and it
does not decay. Raising the proportional gain reduces it and brings the loop
closer to oscillating.

An arm following a path is consistently inside every corner. A drive commanded
along a line is consistently behind where it should be.

## Cause

A feedback loop is a machine for reacting to being wrong, so following a moving
target it must be wrong, continuously, because the error is the only thing
producing the command.

Measured on a 1 kg mass with damping 0.6 and a load of 0.4 N, following a ramp at
0.5 m/s with kp = 20 and kd = 8: **0.234 m**, constant, for ever.

That error has two causes, and the second surprises people.

**The plant needs force to move at all.** Holding 0.5 m/s takes
`0.4 + 0.6 * 0.5 = 0.7 N`, and a proportional term can only produce force from an
error, so it holds `0.7 / 20 = 0.035 m` of error to make it.

**The derivative term opposes steady motion.** Taken on the measurement, which
is the right choice for a step, it contributes `-kd * v = -4 N` at constant
velocity, and the proportional term has to overcome that from an error too.
That is another `4 / 20 = 0.200 m`, and it is the larger of the two.

## Fix

Tell the loop what the target is doing, instead of making it work it out from
being wrong.

```cpp
const double total = rc::control::command(feedback, model, target, lowest, highest);
```

where the model supplies `load + damping * velocity + mass * acceleration`, and
the setpoint carries a velocity and an acceleration rather than only a position.
That removes the plant's share exactly.

The derivative's share is removed by taking the derivative on the error instead,
which has its own price: see `E-CTRL-0010`.

Two things people reach for first, and what they actually do.

**Raising kp.** It divides the error by the gain and does nothing about the
cause, so the lag shrinks in proportion while the loop's stability margin
shrinks with it. Lesson 14-04 has that trade measured.

**Adding an integrator.** It does remove the error, and it takes a detour to get
there: the force has to be built up out of accumulated error, so it lags and
then overshoots. Measured against feedforward on the same ramp, the integrator
reached the same steady error with an excursion twenty times larger on the way.
An integrator is for the part of the load you did not model. It is a poor
substitute for the part you did.

And note that a setpoint carrying only a position throws away both of the things
feedforward needs. Anything generating a trajectory already knows the velocity
and the acceleration; passing them costs nothing.
