id: E-CTRL-0012
title: A setpoint that steps, and an actuator that answers it
match: a step and a profile through the same loop
match: the commanded velocity never jumps
platforms: linux, windows
teaches: 14-06-a-profile-the-machine-can-follow
---

## Symptom

Every commanded move begins with a bang. The actuator saturates, a drive trips
on current, a joint jerks and the load swings. The loop is stable, the move
completes, and the machine sounds like it is being hurt.

Lowering the gains makes it gentler and makes the tracking worse everywhere else.

## Cause

The setpoint jumped, so the loop was told to be somewhere else immediately. An
instantaneous position change is an infinite velocity, and the loop responds
with whatever the actuator has until the error comes down.

Measured on a 1 kg mass with damping 0.6 and a load of 0.4 N, commanding a one
metre move:

| | peak command | worst error |
|---|---|---|
| a step | **20.2 N** | 0.9223 m |
| a profile with feedforward | **1.4 N** | 0.1987 m |

Fourteen times the command, and the peak is set by the gain rather than by
anything about the machine or the move.

The gains are not the problem. They were chosen in lesson 14-04 for a loop that
had to react to disturbances, and a step turns every commanded move into a
disturbance.

## Fix

Move the setpoint at a speed the machine has.

```cpp
const rc::control::Trapezoid profile(distance, top_speed, acceleration);
const auto target = profile.at(elapsed);
```

Speed up at the acceleration limit, hold the top speed, slow down again. The
loop then has almost nothing to correct, because the target is never anywhere
the machine could not already be.

Two things it gives you beyond the gentleness.

**The velocity and acceleration**, which is what feedforward needs and what a
step cannot supply. A profile knows both at every instant because it computed
them, and lesson 14-05 measures what they are worth.

**A duration**, before anything moves. That is a number a caller can plan
around, coordinate against, or refuse.

Two things to get right, and they are `E-CTRL-0013` and `E-CTRL-0014`: the short
move where there is no room to reach the top speed, and the profile that has to
be slowed without being distorted.

The limits themselves come from the machine, not from taste: the top speed from
the drive's rating, the acceleration from what the mechanism and the payload
tolerate. Both are measurable, and both belong beside the profile in the source
with a note saying where they came from.
