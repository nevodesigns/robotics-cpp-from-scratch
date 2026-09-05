id: E-CTRL-0010
title: A derivative term that fights every steady motion, or that explodes on a step
match: two causes, and each fix removes exactly its own
match: what the other derivative costs, on a step
platforms: linux, windows
teaches: 14-05-the-loop-that-only-reacts
---

## Symptom

One of two complaints, from the same choice made two different ways.

**The loop lags a moving target** by much more than the plant's own resistance
explains, and reducing kd reduces the lag while making the loop ring.

Or: **a change of setpoint produces an enormous command spike**, a saturated
actuator, a bang, a tripped drive. The loop is stable before and after and the
transition is violent.

## Cause

A derivative can be taken on the measurement or on the error, and the two differ
exactly when the setpoint moves.

```cpp
derivative = -(measurement - last_measurement) / dt;   // on the measurement
derivative =  (error - last_error) / dt;               // on the error
```

**On the measurement** it cannot spike, because a physical measurement cannot
jump. It also contributes `-kd * v` during any steady motion, which the
proportional term must overcome from an error. That is `kd * v / kp` of standing
lag: 0.200 m in the measured case, the larger part of a total of 0.234.

**On the error** the standing lag is gone, and a step of setpoint moves the error
by the whole step in one sample. Measured on a one metre step at 2 ms:

| derivative taken on | peak command |
|---|---|
| the measurement | 20.4 N |
| the error | **4020.4 N** |

Two hundred times, from the same gain, because the error's derivative was five
hundred metres a second for one sample.

Neither is wrong. They are right about different inputs.

## Fix

Take the derivative on the measurement, and remove the standing lag with
feedforward rather than by moving the derivative.

```cpp
const double total = rc::control::command(feedback, model, target, lowest, highest);
```

Measured, feedforward removes exactly the plant's share of the error and taking
the derivative on the error removes exactly the controller's share, and the two
are independent. So the combination to reach for is feedforward with the
derivative left on the measurement: no spike, and the lag it leaves is the
derivative's share alone.

Where the standing lag still matters after that, the honest options are:

**Ramp the setpoint** instead of stepping it, which is what a trajectory
generator is for, and then the two derivatives agree because the setpoint no
longer jumps.

**Filter the derivative of the error**, which bounds the spike at the cost of
lag in the term itself, and is a tuning problem of its own.

Whichever is chosen, write down which derivative the loop uses and why, next to
the gains. It changes the meaning of kd, so a gain copied between two loops that
differ in this is not the same gain.
