id: E-SENSE-0003
title: A derivative term differentiating the noise
match: too little filtering is an unstable loop
platforms: linux, windows
teaches: 15-01-every-filter-is-a-delay
---

## Symptom

A controller whose output is violent: the command swings between its limits at
the sample rate, the actuator is audible, and the machine gets hot. It roughly
holds the target, and every measurement of it looks terrible.

Turning the derivative gain down helps and gives up the damping it was there
for.

## Cause

The derivative term differentiates whatever it is given, and it does not know
which part of that is the signal.

A measurement with a little noise on it has an enormous **rate of change**: a
millimetre of jitter between two samples two milliseconds apart is half a metre
per second, and the derivative term acts on that as though the robot had lurched.

Measured, with a sensor carrying one centimetre of noise:

| filter | overshoot | settles | command effort |
|---|---|---|---|
| none | 9.2% | never | 17.97 |
| 16 samples | 0.3% | 1.21 s | 3.93 |

Same loop, same gains. The unfiltered version never settles and spends seven
times the command effort doing it, which on real hardware is heat, wear and
noise for nothing.

## Fix

Filter the measurement before the derivative sees it, and choose the window by
measuring rather than by eye. Lesson 15-01 does both.

Two related things worth knowing, because they solve neighbouring parts of the
same problem.

**Take the derivative on the measurement, not on the error.** That is
`E-CTRL-0002`, and it stops a setpoint change from producing a spike. It does
nothing about noise, which is this entry.

**A filter is not free.** Lengthening it until the command looks calm walks
straight into `E-SENSE-0002`: the same loop, filtered with 256 samples instead
of 16, overshoots by 103 percent and never settles. Too little filtering and too
much are both unstable, and the window between them is wide but not infinite.

If the noise is bad enough that no window works, the derivative term may not be
affordable on that sensor at all. A PI controller that is stable beats a PID
that is not, and knowing which you have needs the four numbers from lesson
14-04 rather than an impression.
