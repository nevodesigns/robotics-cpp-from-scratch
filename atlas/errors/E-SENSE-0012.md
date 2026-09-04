id: E-SENSE-0012
title: A dead sensor that keeps reporting, because the timestamp is yours
match: a sensor that freezes, stamped two ways
platforms: linux, windows
teaches: 15-04-the-sensor-that-stopped
---

## Symptom

A device stops, is unplugged, or its firmware hangs, and nothing notices. The
robot keeps acting on the last value it saw, confidently, for as long as it is
left running.

The staleness check that was written for exactly this is passing. Every reading
is milliseconds old.

## Cause

The timestamp is being generated at the moment the reading is used rather than
at the moment it was taken, so it describes your loop rather than the device.

A driver that hands back its last value on every poll, or a buffer that keeps
returning its final entry, produces a stream of readings that are new by your
clock and ancient by any measure that matters. Freshness computed from a stamp
you wrote yourself is a statement about your own liveness.

Measured on a sensor frozen halfway through a run, two channels differing only
in what they were told about when each reading was taken:

- stamped with the device's own time: `stale`, one sampling period after it
  stopped.
- stamped on arrival: `ok`, for ever.

This is the bill for the shortcut in `E-SENSE-0009`. Stamping in the driver
catches the transport latency, which is most of it, and it cannot catch this.

## Fix

Prefer the device's own timestamp wherever the protocol carries one, and treat
its absence as a known gap rather than as an equivalent.

Where there is no device timestamp, detect the freeze from the value instead:

```cpp
Channel channel(calibration, window, max_age, lowest, highest, /*stuck_after=*/8);
if (channel.health(clock.now()) == Health::stuck) hold();
```

An identical reading arriving repeatedly from a sensor with real noise is not
something a live device does. Choose the threshold from the sensor rather than
from habit, because a quiet sensor repeats itself honestly: see `E-SENSE-0013`.

Report `stale` in preference to `stuck` when both apply. A channel that has gone
quiet is old either way, and the age is the more useful thing for a caller to
hear.

And have the caller act on the health rather than only on the value. A driver
that returns a bare number forces every caller to guess what it means, and they
guess "ok".
