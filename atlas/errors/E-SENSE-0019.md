id: E-SENSE-0019
title: A small change that the converter cannot see, and noise that makes it visible
match: noise of about a step makes averaging work, because it moves the input across the boundaries
match: a change of a third of a step is invisible without noise two times in three, and visible with it
platforms: linux, windows
teaches: 15-08-counts-not-volts
---

## Symptom

A slow drift in a sensor is invisible for a long time and then appears as a sudden jump of one count. A
measurement that should see a tiny change sees it sometimes and not at other times, depending on where the
signal happened to start, and a quieter sensor is *worse* at it than a noisier one.

## Cause

With no noise, a change smaller than a step is seen only when it happens to carry the voltage across a
boundary between two codes. A change of a third of a step does that a third of the time, and when it does,
the code moves by a whole step: three times too much.

Noise moves the input across the boundaries from reading to reading, so the fraction of readings on each
side of a boundary follows where the input really is inside the step, and the average carries it.

Measured, a change of a third of a step noticed by the average of 1000 readings:

| | noticed |
|---|---|
| no noise on the input | 31.17 percent of the time |
| a step of noise | 100.00 percent of the time |

And the rms error in steps of the average of n readings with noise of sigma steps:

| noise | n = 1 | n = 100 | n = 1000 |
|---|---|---|---|
| 0.0 | 0.2817 | 0.2817 | 0.2817 |
| 0.3 | 0.4023 | 0.0550 | 0.0389 |
| 1.0 | 1.0489 | 0.1052 | 0.0348 |
| 3.0 | 3.0155 | 0.3094 | 0.1012 |

Noise of 0.3 of a step is already enough to start. About a step is where it works as one over the square
root of n. More than that it is averaged away, but from further out.

## Fix

If a signal changes by less than a step and matters, add noise on purpose before the converter, of about one
step, which is called dither, and average. A small triangular dither is the usual choice. It costs the single
reading its accuracy, since one reading is now wrong by about the noise and not by 0.29 of a step, and buys
resolution in the average. Whether the trade is worth it depends on whether the readings are used singly or
together.
