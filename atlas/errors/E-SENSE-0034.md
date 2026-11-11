id: E-SENSE-0034
title: A line drawn across a hole in the stream
match: across a hole in the stream no line is drawn, however many instants fall in it
platforms: linux, windows
teaches: 15-15-two-clocks-one-timeline
---

## Symptom

A sensor that dropped out for half a second has a perfectly smooth trace through the gap, as if it had been working.

## Cause

Two samples far apart have a line between them whatever happened in between. With a hole from 1.0 s to 1.6 s in a 10 Hz stream, a limit of 0.15 s answered 192 of 251 instants and none of the 59 inside the hole, and no limit answered all 251.

## Fix

Give interpolation a max_gap a little over the sample period.

```cpp
const auto aligned = resample(stream, times, 0.15);
```
