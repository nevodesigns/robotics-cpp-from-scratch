id: E-SENSE-0017
title: A converter's range divided into the wrong number of steps
match: the steps, the codes and the middle of a step, with the convention stated
match: outside the range the converter clips, and says nothing about how far out
platforms: linux, windows
teaches: 15-08-counts-not-volts
---

## Symptom

Every voltage read through the converter is a little wrong, by an amount that grows towards the
top of the range. The error is under one percent at the bottom and a good deal more at the top, so
a calibration at low voltages passes and one at high voltages does not. Or a reading that should
be the top of the range is one code past it.

## Cause

The range was divided by `2^bits - 1` and not `2^bits`. The converter cuts the range into `2^bits`
steps, so the highest code is `2^bits - 1` and the very top of the range is the top edge of that
last step, not a code of its own. With three bits over zero to eight volts there are eight steps of
exactly one volt. Dividing by seven makes the step 1.143 V, and then the code for 1.0 V is 0 where it
should be 1.

A second, related thing: a voltage below the range gives the lowest code and a voltage above it gives the
highest, and nothing else. A thousand volts and nine volts are the same reading. The converter clips.
It does not wrap and does not report an error, so a reading at either end is ambiguous about how far out
the input was, and `saturated` is how code that uses it can know.

## Fix

State the convention and test the edges: code 0 for 0.0 and 0.999 V, code 1 at exactly 1.0 V, code 7 at
7.999 V, and code 7 again at 8.0, 9 and 1000 V. And a code stands for the middle of its step, so
`value(code) = low + (code + 0.5) * lsb`, which is the choice that makes the error at most half a step
either way.
