id: E-CTRL-0015
title: A gain that was fine, until the loop got a little late
match: what a delay does to a gain that was fine without it
match: expected .*margin > 0.0.* to equal
platforms: linux, windows
teaches: 14-07-the-delay-in-the-loop
---

## Symptom

A loop that was tuned on the bench shakes itself apart in the robot. The same gains.
It rings, then the ring grows, then the actuator hits a limit and the loop settles into
a violent steady oscillation. Making the gain smaller fixes it, and nobody can say how
small is small enough.

The thing that changed was not the controller. It was something between the sensor and
the motor: a filter, a message over a link, a sensor that reports a moment after it
reads.

## Cause

A delay adds no gain, so nothing about the loop looks bigger. It adds lag, and lag grows
with frequency: a delay of `d` seconds is `360 f d` degrees at `f` hertz. At the
frequency where the loop gain falls to one, the crossover, the loop has a margin before
the lag turns negative feedback into positive. A delay uses it up.

Measured, a proportional gain of 3000 with critical damping on a pure inertia of 0.5
kg m^2, in a 1 kHz loop:

| delay | phase margin | the step response |
|---|---|---|
| 0 ms | 71.8 degrees | settles |
| 5 ms | 26.1 degrees | settles |
| 10 ms | -19.6 degrees | grows without bound |
| 20 ms | -110.9 degrees | grows without bound |

Five milliseconds is not much, and it takes a margin of 72 degrees down to 26. Ten takes
it below zero.

## Fix

Know the delay in the loop, add it up, and compute the margin before the robot does:
sensor latency, filter lag (a filter is a delay, which phase 15 measures), the age of the
message, and half a sample for holding the command. Then keep the gain where the margin is
positive with room, which is what `largest_stable_gain` computes. A margin of 45 degrees
or more is the usual target, because the real delay is never known as exactly as the
model's.
