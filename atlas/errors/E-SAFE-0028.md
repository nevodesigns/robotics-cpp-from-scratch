id: E-SAFE-0028
title: A forgotten input that was taken as fine
match: when nothing is known, the answer is stop, and everything is a cause
platforms: linux, windows
teaches: 19-10-one-place-that-says-stop
---

## Symptom

A new monitor is added and its field is never filled in, and the supervisor carries on as if it were healthy.

## Cause

An Inputs that nobody filled in gave stopped, a cap of 0 and all six causes: estop, link, fence, tilt, sensors, power. A reset with those inputs was refused, and with a healthy set it was accepted and the next step was normal.

## Fix

Make every default the unsafe one.

```cpp
bool estop_closed = false;
rc::safety::TiltState tilt = rc::safety::TiltState::stop;
```
