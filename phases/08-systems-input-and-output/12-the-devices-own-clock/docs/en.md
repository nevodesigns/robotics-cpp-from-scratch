# The Device's Own Clock: Ticks That Wrap and Drift

> A device's timestamp is in its own clock, which wraps, starts anywhere, runs a
> little fast and is sometimes reset. Take all four out before trusting a time.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 08-11, 03-05

## The Problem

A message from a device says what it measured and when, and the "when" is in the
device's clock. Nothing a robot does with a time is right until that clock has been
related to its own: fusing an IMU with a camera, working out a velocity from two
positions, deciding a reading is stale.

The shortcut is to ignore the device's stamp and use the time the message arrived.
That is wrong by however long the message was delayed, which on a link with jitter is
different every time. The other shortcut is to take the offset between the clocks once
and use it for ever, which is right at the start and wrong a little more every second
afterwards.

Four things are wrong with a device's stamp, and each has its own remedy.

## The Concept

### It wraps

A device counts in a fixed number of bits. Sixteen bits of milliseconds is 65.5
seconds before it starts again at zero, and a robot that subtracts two stamps across
the wrap gets a duration of minus a minute.

```cpp
int dt = now_stamp - last_stamp;        // both are std::uint16_t
```

A `uint16_t` is promoted to `int` before the subtraction, so `5 - 65531` is `-65526`
and not `10`. A centimetre in ten milliseconds, which is a speed of 1 m/s:

| | duration | speed |
|---|---|---|
| subtracted in `int` | -65526 ms | -0.00015 m/s |
| unwrapped | 10 ms | 1.00 m/s |

The bad one is a clean, small, plausible number. It happens once every 65.5 seconds,
which nobody sees in a hand test and everybody sees in a ten minute run. See
`E-IO-0037`.

`TickUnwrapper` takes the difference **in the counter's own width**, so it wraps the
way the counter does, and adds it to a 64 bit running total that does not. It is the
same arithmetic as the sequence number in lesson 08-08, applied to a clock.

### It is reset

A serial link delivers in order, so a stamp that goes backwards is not a message that
arrived late. It is the device starting again.

The unwrapper says so, with `restarted`, and carries the timeline on from where it
was. A counter can see a gap of just under half a turn, 32767 ticks, which is 32.7
seconds for sixteen bits of milliseconds. A silence longer than that looks like a
restart, and a restart looks like a silence. See `E-IO-0039`.

### It starts anywhere, and runs a little fast

Unwrapping gives a clock that runs forward. It is still not the host's: it started at
some arbitrary point and its oscillator is a little off, a good crystal by tens of
parts per million and a cheaper one by thousands. So the device-to-host relation is a
line, `host = slope x device + intercept`, and the slope is not one.

What the host observes is the **arrival** time, which is the send time plus a delay,
and the delay is never negative. So the best line lies **under** the arrival times and
touches the least delayed ones.

`ClockMap` keeps the least delayed sample in each ten seconds of device time, a
bucket, and fits a line by least squares through the buckets of the last ten minutes.
It needs two buckets before it has a slope, and with one it is an offset only.

### Measured

A device 50 ppm fast, stamping a 100 Hz stream for an hour on sixteen bits of
milliseconds, through a link that delays each message by two to ten milliseconds. The
worst error in the send time of a message, after the first ten minutes:

| using | worst error |
|---|---|
| when the message arrived | 8.0 ms |
| the first message's offset, for ever | **185.3 ms** |
| the fitted map | **0.84 ms** |

The arrival time is wrong by the jitter, which does not grow. The fixed offset is
wrong by `50e-6 x 3600 s = 180 ms` and by more the longer the program runs. The map
found a drift of 49.9 ppm for a device that was 50.0. See `E-IO-0038`.

The error is measured from the send time **plus the least delay**, 2 ms, because that
is the most anyone on one side of a link can know. The smallest delay cannot be
separated from the clock offset without a second clock, which is why a network time
protocol assumes the two directions take the same time. For fusing two devices on one
host the constant cancels. For absolute time it does not, and the lesson says so
instead of hiding it.

### A restart must reach the map

The unwrapped timeline carries on across a reset and the seconds the device was off
are not in it, so every old bucket is that far out, and the fit averages them in for
ten minutes.

A device off for five seconds at thirty minutes, restarting its counter. The worst
error over the next ten minutes:

| | worst error |
|---|---|
| map kept, restart not noticed | **5011.0 ms** |
| map reset on the restart | **3.5 ms** |

The error is the downtime, to the millisecond. The cure is one line:

```cpp
const Unwrapped u = unwrapper.accept(raw);
if (u.restarted) map.reset();
```

## Build It

`TickUnwrapper` and `ClockMap` in `exercise/solution.hpp`. The members are given and
the comments in each function list the moves.

```
rcpp verify 08-12
```

The first five tests are the unwrapper: through a wrap, a duration across it, a
thirty two bit counter, a restart, and the half turn limit. Then the map: exact when
there is no jitter, and an offset only with one bucket. The last two are the tables
above, each an hour of simulated device on a virtual clock, so they take a moment and
give the same answer every time.

## Use It

`rc/io/timesync.hpp` joins `rc::io`:

```cpp
rc::io::TickUnwrapper<std::uint16_t> unwrapper;
rc::io::ClockMap map;

// for every message
const rc::io::Unwrapped u = unwrapper.accept(raw_stamp);
if (u.restarted) map.reset();
map.add(u.ticks / 1000.0, arrival_seconds);
const double sent_at = map.to_host(u.ticks / 1000.0);
```

Two things to carry away. **Subtract in the counter's width**, wherever a counter is
wrapped, which is the same lesson as the sequence number. And **do not trust a
constant**: an offset measured once is a measurement of one instant.

What this does not do is know the absolute delay, or survive a device whose clock is
being steered, because a clock that is corrected by the device itself has a slope that
changes, and a fit over ten minutes will be wrong during the change.

## What Breaks First

- **Subtracting stamps in `int`.** A negative duration once per wrap. See
  `E-IO-0037`.
- **A fixed offset.** It is right at the start and drifts by the oscillator's error
  for as long as the program runs. See `E-IO-0038`.
- **A restart nobody told the map about.** Every timestamp is wrong by the downtime
  for ten minutes. See `E-IO-0039`.

## Ship It

`rc/io/timesync.hpp` holds `Unwrapped`, `TickUnwrapper` and `ClockMap`.

The phase now has every piece of a conversation with a device: bytes that are
specified, frames that are found and checked, a link that survives loss and noise, a
reader that cannot be held up, requests that are matched to answers, and time that
means something. Two things are left. A cable that comes out and goes back in, and
all of it together.
