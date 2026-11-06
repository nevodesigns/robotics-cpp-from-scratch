id: E-SENSE-0016
title: A real change, thrown away as a spike
match: a real step looks like a spike until half a window agrees, and a floor on the spread helps a small one
match: on a step a median stays sharp and a mean smears it over the whole window
platforms: linux, windows
teaches: 15-07-the-sensor-that-is-sometimes-wildly-wrong
---

## Symptom

A sensor that was perfectly steady changes, truly, and the filtered signal does not
follow. It stays where it was for several readings and then jumps to the new value. Or, on
a very quiet sensor, a real change a few counts large is rejected over and over.

## Cause

On the first reading of a step, a step and a spike are the same thing: one number far from
the rest. Only the readings after it say which it was, and the filter cannot know yet. So
it waits until half the window agrees.

Measured, a Hampel filter of nine on a step from 0 to 1 on a sensor that had been perfectly
quiet: held back for **4 readings**. A median of nine behaves the same way, arriving 4
samples late but then rising in a single sample (a mean of nine takes 8).

Quiet sensors make it worse. A window with no spread at all has a median absolute deviation
of exactly zero, so a threshold of three times it is zero, and every change however small
is infinitely far away. A change of 0.003 on a sensor with a resolution of 0.001 was
rejected for 4 readings with no floor on the spread and for none with a floor of 0.001.

## Fix

Give the filter the one thing the window cannot know, the least the sensor can resolve, as
a floor on the spread: `scale = max(1.4826 * mad, min_scale)`. And accept the delay of
half a window on a genuine step as the price of not passing a spike: it is the same
`(N - 1) / 2` as the breakdown point, because they are one fact. If a step must be seen
faster than that, the answer is a second sensor, not a cleverer filter.
