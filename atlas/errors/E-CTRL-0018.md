id: E-CTRL-0018
title: A loop that never settles, because the joint sticks and the integral keeps pushing
match: without an integral the loop sticks short, and with one it can hunt
match: expected this to be true: medium.report.hunting
platforms: linux, windows
teaches: 14-08-stuck-then-slipping
---

## Symptom

A joint that should hold a position instead creeps up to it, stops short, jumps past it,
stops again on the other side, and does it again, slowly, for as long as the program
runs. The amplitude is small and the period is seconds. It is worse with more integral
gain and it is absent on the bench with the load off.

The first suspicion is a bad tuning. It is not: the same gains settle on a joint without
friction.

## Cause

Static friction holds a mass still with a force up to a limit, then lets go at once, and
the force it takes to keep the mass moving is smaller than the force it took to start it.
A proportional controller alone stops pushing harder once the error is small, so the mass
sticks short of the target, anywhere inside a band: the breakaway force over the gain.
An integral keeps pushing, and that is where it goes wrong.

The integral winds up until the force passes the breakaway. The mass lets go, with more
force than it needs to keep moving, accelerates through the target, and the error changes
sign. The integral begins to unwind. The mass slows, sticks on the far side, and the whole
thing happens again the other way. That is a limit cycle, and nothing in the controller is
wrong: it is the controller and the friction together.

Measured, a 1 kg mass with a breakaway of 1.5 N and a Coulomb friction of 1.0 N held at one
metre, a proportional gain of 20 and damping of 2:

| controller | final error | peak to peak | crossings | |
|---|---|---|---|---|
| no integral | 0.0623 | 0.0000 | 0 | settled, short |
| integral gain 5 | 0.0138 | 0.0000 | 0 | settled |
| integral gain 20 | -0.0196 | 0.0397 | 3 | hunting |
| integral gain 50 | -0.0249 | 0.0499 | 8 | hunting |

Without the integral the error sits at 0.0623, inside the 1.5 / 20 = 0.075 the breakaway
allows. A small integral closes it. A bigger one hunts, and the bigger it is the harder.

## Fix

Detect it, then decide. `analyze_hunting` looks at the error over a stretch of time and
reports a peak to peak swing and the number of crossings of its own mean: a loop is
hunting if both are large, and a steady offset, however big, is not hunting. The ways to
stop it are in `E-CTRL-0020`, and none of them is free.
