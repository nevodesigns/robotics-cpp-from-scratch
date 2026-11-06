id: E-SENSE-0014
title: A mean over a sensor that is now and then wildly wrong
match: what each costs, on a slow signal with noise and one reading in a hundred wrong
match: a spike moves a mean by its size over the window, and a median not at all
match: expected this to be true: median_rms
platforms: linux, windows
teaches: 15-07-the-sensor-that-is-sometimes-wildly-wrong
---

## Symptom

A smoothed reading that is still thrown off, for a moment, by nothing: a distance that
jumps to a metre and back, a temperature that spikes, an encoder that reports a million
counts once an hour. The filter is working. It has been averaging, and the average is
doing what an average does.

## Cause

A mean moves by the size of a bad reading over the number of readings. A spike of 100 in
a window of nine moves it by 11, for nine readings. The worse the reading, the further it
moves, and a sensor that is occasionally wildly wrong is wrong by a large and unbounded
amount, so the average inherits it.

A median counts which side of the middle a reading falls, and not how far, so a spike
moves it by nothing at all.

Measured, a sine of amplitude 1 with noise of 0.05, 211 of 20000 readings wrong by 50 to
100:

| filter | rms error against the truth |
|---|---|
| none | 7.681 |
| mean of nine | 2.519 |
| median of nine | 0.092, or 0.025 once its four samples of delay are allowed for |
| Hampel, nine, three sigma | 0.053 |

The mean turned an error of 7.7 into one of 2.5: smaller, and still wrong by fifty times
the noise. The median is nearly right.

## Fix

Use a median, or a Hampel filter, for a sensor that has wild readings, and a mean only
for one whose noise is steady and bounded. The two answer different questions. A median of
nine also reduces ordinary noise, because it is an average of sorts, and a Hampel filter
leaves ordinary noise as it found it: 0.053 is almost exactly the 0.05 that went in.
Which is right depends on whether the noise or the spikes are the problem.
