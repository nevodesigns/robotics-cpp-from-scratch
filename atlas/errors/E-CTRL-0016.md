id: E-CTRL-0016
title: A stability limit that says there is none
match: the margin predicts the largest stable gain, and the gain falls as the square of the delay
match: expected this to be true: rows.*simulated
platforms: linux, windows
teaches: 14-07-the-delay-in-the-loop
---

## Symptom

A calculation says the loop is stable at any gain, with no delay, and the loop is not. Or
it gives a limit that the loop beats by a few percent in one direction and by two and a
half times in the other, and nobody knows which to believe.

## Cause

The model has no limit without a delay, and a real loop always has one. A command is
computed, held, and applied until the next sample, so it arrives on average half a sample
late, and the controller's own computation takes time. The model has to be told, or it
answers a question about a loop that does not exist.

Measured, a 0.5 kg m^2 inertia in a 1 kHz loop, the largest proportional gain at which a
step settles, by simulation and by the margin with half a sample added to the delay:

| delay | simulated limit | predicted | difference |
|---|---|---|---|
| 0 ms | 341737 | 838222 | 145.3 % |
| 5 ms | 6627 | 6928 | 4.5 % |
| 10 ms | 1852 | 1901 | 2.6 % |
| 20 ms | 490 | 499 | 1.8 % |
| 40 ms | 125 | 128 | 1.8 % |

With a delay the prediction is within a few percent. With none it is off by a factor of
two and a half, because the limit there is the sampling itself, which the model of a
continuous loop with a pure delay does not describe well: a discrete controller, a held
command and an integration step are all there and the model has one number for them.

The law is in the column: each doubling of the delay cuts the limit about fourfold, 3.6,
3.8, 3.9. The gain a loop can stand goes as the inverse square of its lateness.

## Fix

Include half the sample period in the delay, as the test does, and treat the answer at
very small delays as an upper bound, not a figure. Where the margin matters, simulate
the actual discrete loop, which is a few lines and gives the truth, and use the margin to
find out in which direction to look.
