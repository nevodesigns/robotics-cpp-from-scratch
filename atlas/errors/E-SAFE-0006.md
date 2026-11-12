id: E-SAFE-0006
title: A timeout that a clock set back can never reach
match: a clock that goes backwards is a stale heartbeat, not an immortal one
platforms: linux, windows
teaches: 19-02-the-stop-that-does-not-need-the-app
---

## Symptom

After a time synchronisation the robot ignores a dead link for minutes, or for ever.

## Cause

A heartbeat at 100 s and a clock of 50 s give now minus last of -50, which is not above the timeout of 0.3. With the test now < last_beat counted as stale, the output was 0 and stale was true.

## Fix

Treat time that goes backwards as stale, and use a clock that cannot be set.

```cpp
return !beat_seen_ || now < last_beat_ || (now - last_beat_) > timeout_;
```
