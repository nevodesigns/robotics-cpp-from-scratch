id: E-CTRL-0011
title: A feedforward model treated as though it had to be right
match: a rough model beats no model, in both directions
match: the total is limited once, not twice
platforms: linux, windows
teaches: 14-05-the-loop-that-only-reacts
---

## Symptom

Feedforward is discussed and not implemented, because nobody knows the mass, or
the friction is temperature dependent, or the load changes with what the robot
is carrying. The loop keeps its standing lag and somebody raises the gains
instead.

Or it is implemented, and the command saturates in a way it did not before, or
the limit is applied in a place that quietly discards it.

## Cause

Two separate mistakes about what a model is for.

**Treating it as an all or nothing.** The error a feedforward leaves is
proportional to how wrong it is, and there is no cliff. Measured, following a
ramp with the damping term estimated wrongly:

| guess / true | steady error |
|---|---|
| 0.0 | 0.014000 |
| 0.5 | 0.006500 |
| 1.0 | 0.001000 |
| 1.5 | 0.008500 |
| 2.0 | 0.016000 |

With no feedforward at all the error was **0.234000**. Leaving the damping term
out entirely and feeding forward only the load and the inertia was still sixteen
times better than nothing, and the penalty is symmetric: guessing high costs the
same as guessing low.

**Limiting in the wrong place.** Clamping the feedback and then adding the
feedforward gives a total that exceeds the limit. Clamping each separately
throws away authority the actuator has. Either way the number that reaches the
motor is not the number anybody chose.

## Fix

**Model what you can and feed forward what you model.** The load is usually
known to within a few percent from a static measurement; the inertia from a
drawing; the damping from one run at constant speed. Any of the three alone is
worth having.

**Limit once, at the end.**

```cpp
const double total = feedback + model.force_for(target.velocity, target.acceleration);
return clamp(total, lowest, highest);
```

**Watch for the feedforward reaching the limit on its own.** That is a useful
signal rather than a fault: it means the profile is asking for more than the
machine has, before any error has occurred, and the right response is to slow
the profile rather than to let the loop discover it as a tracking failure.

**Leave an integrator for what you did not model.** Feedforward handles the part
you can predict and the integrator mops up the drift, the temperature and the
unknown payload. Using the integrator for the predictable part means it is
always winding, which is the slow, overshooting path to an answer feedforward
gives immediately.

And measure the model rather than trusting the data sheet. One run at a constant
speed gives the load and the damping together, two runs at two speeds separate
them, and a run with a known acceleration gives the inertia. That is an
afternoon, and the table above is what it buys.
