id: E-SENSE-0015
title: A median asked to outvote more than half its window
match: a median fails when more than half the window is bad, and not before
match: expected .*worst.* to be within .* of 100
platforms: linux, windows
teaches: 15-07-the-sensor-that-is-sometimes-wildly-wrong
---

## Symptom

A filter that has been removing spikes for months lets one straight through, and holds it
for as long as it lasts. It happens when the sensor has a bad patch: a cable that works
loose and chatters, a reflection that gives a wrong range for a few cycles, a link that
repeats a stale value.

## Cause

A median is the middle of a sorted window, so it is right as long as the bad readings are
fewer than half of it. At half and above they are the middle.

Measured, a median of nine and a burst of consecutive readings of 100 among zeros:

| burst length | worst output |
|---|---|
| 1 | 0 |
| 2 | 0 |
| 3 | 0 |
| 4 | 0 |
| 5 | 100 |
| 6 | 100 |

Four are outvoted and the fifth is a majority. The limit is half the window, rounded down:
`(9 - 1) / 2`. This is called the breakdown point of the median, and it is as good as a
location estimator can be, and it is not infinite.

## Fix

Choose the window from the longest burst the sensor can produce, not the noise: to survive
`b` bad readings in a row the window needs at least `2b + 1`. And accept what it costs.
A window of nine delays a signal by four samples, so surviving longer bursts is paid for in
lag, which is the trade lesson 15-01 measured for every filter. A burst longer than the
window should be detected as a fault, with the staleness check of lesson 15-04, and not
filtered.
