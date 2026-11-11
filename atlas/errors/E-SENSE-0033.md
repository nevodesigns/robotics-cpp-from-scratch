id: E-SENSE-0033
title: A value made up outside the stream
match: before the first sample, after the last, and with nothing at all, no value is made up
platforms: linux, windows
teaches: 15-15-two-clocks-one-timeline
---

## Symptom

A reading is reported for an instant the stream does not cover, and it looks plausible, so nothing catches it.

## Cause

Interpolation joins two samples. Before the first sample or after the last there is only one side, and a value there is an extrapolation passed off as data. The call refused for -0.01 s, 1.01 s on a stream of 0 to 1 s and for an empty stream, and left the output as it was.

## Fix

Check the answer and treat a refusal as no data.

```cpp
double v;
if (interpolate_at(stream, t, max_gap, v)) use(v);
```
