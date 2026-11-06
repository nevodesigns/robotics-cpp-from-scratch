id: E-CTRL-0021
title: One position loop that must do everything, and overshoots or sags
match: a move of two radians, and then a load: one loop against two
match: expected this to be true: pid.overshoot
platforms: linux, windows
teaches: 14-09-two-loops-one-inside-the-other
---

## Symptom

A joint that is told to make a large move overshoots it by a large amount, or crawls to it, and when a load arrives
it either sags and stays there or takes seconds to come back. Changing the gains trades one fault for the other:
more integral cures the sag and makes the overshoot worse, and less integral does the opposite.

## Cause

A single position loop is asked to do three jobs with three gains. It has to move fast, hold against a load, and not
overshoot, and one integral term cannot serve all of them. The proportional and derivative gains alone leave a steady
sag under a load, the load over the gain. Add an integral to remove it and the integral winds up during the large move,
when the motor is saturated and the error is big, and has to be unwound on the far side, which is overshoot.

Measured, a joint of 0.5 kg m^2 asked to move 2 radians with a motor of 20 N m, held, and at eight seconds a steady
load of 5 N m arrives:

| controller | overshoot | settled | peak speed | worst error after the load | final |
|---|---|---|---|---|---|
| position loop, P and D | none | 0.61 s | 8.04 rad/s | 0.0250 rad | 0.0250 rad |
| position loop, PID | 14.2 % | 3.18 s | 8.83 rad/s | 0.0208 rad | 0.0002 rad |
| cascade, speed limit 3 | none | 0.81 s | 3.04 rad/s | 0.0051 rad | 0.0000 rad |

The proportional loop sags by 25 mm and stays. The PID overshoots by 14 percent and takes 3.2 seconds. The cascade
does neither.

## Fix

Split the jobs between two loops. The outer loop decides how fast the joint should be going. The inner loop decides what
torque gets it there, and holds that speed against a load. The integral lives in the inner loop, where a large move
does not wind it up, and `E-CTRL-0023` says why it still needs a guard.
