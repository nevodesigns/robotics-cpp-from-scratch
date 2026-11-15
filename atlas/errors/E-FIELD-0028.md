id: E-FIELD-0028
title: An animation whose speed is the rate of the log
match: the replay of a log with a gap plays without the gap
match: key times are the sample index
match: the world is flipped so that y is up, the view starts at minus the top, and the playback follows the speed
platforms: linux, windows
teaches: 22-09-watching-a-run
---

## Symptom

A pause in the log does not show as a pause in the replay, and a replay takes the wrong time. The log has gaps or a varying rate.

## Cause

The animation used the sample index as time. The run lasted 9.98 s, and should play in 2.50 s at four times.

An animation keyed by sample index plays one sample per frame, so a gap of a second plays as one frame and a burst of samples plays slowly. The time stamps are the time.

## Fix

Take the key times from the time stamps, and divide the span by the speed.

```cpp
const double dur = (ts.back() - ts.front()) / speed;
```
