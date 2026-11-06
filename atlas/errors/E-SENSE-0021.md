id: E-SENSE-0021
title: A window long enough to see the counts, and a loop that now runs late
match: a window pays for its resolution in delay: a change of speed arrives half a window late
match: a window of counts gives the speed over the window, and less until it has filled
platforms: linux, windows
teaches: 15-09-speed-from-counts
---

## Symptom

The speed estimate is smooth and accurate at a steady speed, and it trails behind whenever the speed changes: a start, a stop,
a reversal. A loop built on it is sluggish, or rings, after the window was lengthened to cure noise.

## Cause

A window counts the counts over a number of samples, so its resolution is a count over the whole window, and what it reports
is the average speed over the window. The average over a window is the speed half a window ago.

Measured, a speed that doubles at sample 200 with a window of 50: the estimate reaches the halfway value 24 samples later, half
a window, 25 ms. That is lesson 15-01's result, that every filter is a delay, arrived at from the other side: the window is a moving average
of the speed, and has all of its cost.

At 0.05 rev/s the window of 50 was still 20 percent out: one count per 20 samples, so a window of 50 holds two or three, and a
count more or less is a third of the answer. A longer window would fix that and delay more.

Until the window has filled it has to use what it has: the oldest count it holds and the number of samples between, and never
divide by a window it has not seen, or the first readings are too low by the fraction of the window missing.

## Fix

Choose the window from the slowest speed that matters and the delay the loop can stand, and accept that it is a trade. Where a
slow speed has to be known quickly, do not widen the window: use the edge timing, which has a resolution of a clock tick, not a
count.
