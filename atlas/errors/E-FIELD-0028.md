id: E-FIELD-0028
title: An animation whose speed is the rate of the log
match: the replay of a log with a gap plays without the gap
match: key times are the sample index
match: the world is flipped so that y is up, the view starts at minus the top, and the playback follows the speed
platforms: linux, windows
teaches: 22-09-watching-a-run
---

## Symptom

A pause in the log does not show as a pause in the replay, and a run played at the sample rate takes the wrong time.

## Cause

The animation used the sample index for time. The run lasted 9.98 s, and 2.50 s at four times.

## Fix

Take the key times from the time stamps and divide the span by the speed.

```cpp
const double dur = (ts.back() - ts.front()) / speed;
```
