id: E-TIME-0001
title: A negative or enormous elapsed time
match: expected .*dt.* to be
match: quality == TickQuality::Backwards
platforms: linux, windows
teaches: 03-05-time-and-clocks
---

## Symptom

A controller produces a large sudden output for one cycle, or an interval comes
back negative, or a timeout that should be milliseconds turns out to be decades.
It happens rarely and is nearly impossible to reproduce on demand.

## Cause

Elapsed time was measured with std::chrono::system_clock, the wall clock. It can
be set by an administrator, stepped by a time synchronisation daemon when a
machine's clock drifts, and moved by daylight saving. A step backwards makes now
minus last negative.

If that difference is computed in an unsigned type it does not come back
negative, it wraps to an enormous positive number, which is how a clock
adjustment turns into a lurch rather than a warning.

## Fix

Measure durations with std::chrono::steady_clock, which never goes backwards.
Keep system_clock for recording when something happened, in a log line or a
timestamp somebody reads. A robot uses both, for different jobs.

Then refuse a nonsense interval rather than passing it on:

```cpp
if (dt <= 0.0) return TickResult{0.0, TickQuality::Backwards};
if (dt > max_dt) return TickResult{max_dt, TickQuality::Stalled};
```

Testing this needs an injected clock, because a real clock cannot be asked to
step backwards on demand.
