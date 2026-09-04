# The Sensor That Stopped: A Channel That Says What It Knows

> The check rejected nothing, ever, and the value twitched every time the sensor
> dropped out. Both of those were the same line, in the wrong place.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 15-02, 15-03

## The Problem

This phase has taken a sensor apart three times. Lesson 15-01 dealt with the
part of its error that is random and what removing it costs. Lesson 15-02 dealt
with the part that is not random and that no amount of averaging reaches. Lesson
15-03 dealt with the part that is not in the value at all.

Each of those was a separate correction with a separate test. On a real robot
they are one function, called at 500 Hz, and two questions arrive with them that
none of the three lessons asked:

**Can this reading be trusted at all?** A device drops a sample, saturates,
returns its buffer's last entry after being unplugged, or hangs while its driver
keeps politely answering.

**How late is it?** Not the transport alone: the filter adds its own delay, and
the loop downstream is spending both from one budget.

A driver that returns a bare number leaves every caller to guess at those, and
they guess well.

## The Concept

### The order that is forced

Three operations: check the reading, calibrate it, filter it. Six possible
orders, and only one constraint, but it is a hard one.

**The check has to come before the filter.** Here is one dropout reporting zero
counts, in an otherwise steady stream at 5 m:

| step | raw | checked first | filtered first |
|---|---|---|---|
| 19 | 5.3450 | 5.0000 | 5.0000 |
| 20 | 0.0000 | 5.0000 | 4.6609 |
| 21 | 5.3450 | 5.0000 | 4.6609 |
| 22 | 5.3450 | 5.0000 | 4.6609 |

| | checked first | filtered first |
|---|---|---|
| refused the dropout | 1 | 0 |
| worst error | 0.0000 m | 0.3391 m |
| outputs spoilt after it | 0 | 15 |

Calibrated, the dropout is -0.4264 m. That is not a distance and any check
catches it. Averaged over sixteen samples with fifteen good readings it is
4.6609 m, which is an ordinary distance, and **the check that comes after the
filter can never fire**. It reports zero rejections for the life of the robot
while sixteen outputs carry each bad reading.

The other two commute. Calibrating then filtering, against filtering then
calibrating, over five hundred readings: the worst difference is 1.8e-15,
because a straight line through an average is the average of a straight line.
Calibrate first anyway, so that the range check and everything below it are in
metres rather than counts.

### Limits from the physics

A range check is only as good as its limits, and the useful ones come from what
the world can do rather than from what the number can hold.

A room is not four hundred metres long. A wheel does not turn at ten thousand
radians a second. Limits taken from the ADC's range reject only values the ADC
could not have produced, which is none of them.

Write the test as the requirement and negate it:

```cpp
if (!(calibrated >= lowest && calibrated <= highest)) { ++rejected; return false; }
```

That phrasing rejects a nan. The other one accepts it, for the reason catalogued
as `E-SENSE-0007`.

And a refused reading must change nothing: the filter does not see it, the
timestamp does not advance, and if refusals continue the channel goes stale on
its own. That is the honest outcome, and it is more useful than a value held
alive by readings that were all rejected.

### A dead sensor stamped with your own clock

Lesson 15-03 said to stamp in the driver when the device offers no timestamp of
its own. Here is what that shortcut costs.

A device freezes halfway through a run and its driver keeps returning the last
value it saw. Two channels, identical except for what they were told about when
each reading was taken:

- stamped with the device's own time: **stale**, one sampling period after the
  freeze.
- stamped on arrival: **ok**, for ever.

Freshness computed from a stamp you wrote yourself is a statement about your own
liveness, not the device's. Every reading is milliseconds old, and every reading
is the same dead number.

So: prefer the device's timestamp whenever the protocol carries one, and where
it does not, detect the freeze from the value instead.

### How long a healthy sensor repeats itself

The backstop is a repeat counter: identical readings arriving from a sensor with
real noise are not something a live device does.

Whether that is true depends on the sensor. The longest run of identical
readings from a parked, healthy device, over a hundred thousand samples at 1 mm
resolution:

| noise | noise / resolution | longest run |
|---|---|---|
| 0.0500 m | 50 | 3 |
| 0.0100 m | 10 | 4 |
| 0.0020 m | 2 | 6 |
| 0.0010 m | 1 | 11 |
| 0.0005 m | 0.5 | 23 |
| 0.0001 m | 0.1 | 100000 |

At fifty times the resolution, three is the most that ever happened and a
threshold of eight is comfortable. **Below its own resolution a parked sensor
repeats for ever and is working perfectly**, and no threshold is both useful and
correct.

Measure the run length on the device you have. A threshold that came from a
habit is a threshold nobody measured, and it will be wrong on the next sensor by
the ratio between the two.

### A channel that reports its own lag

The last thing a channel owes its caller is the cost of everything above.

```cpp
double lag_seconds(rc::core::Nanoseconds now, double sample_interval) const;
```

The filter's own lag, half its window, plus however old the newest reading
already was. Lesson 15-03 measured that the loop cannot tell those two apart, so
adding them is the only honest way to report them.

It also fixes something subtle. The timestamp on a filtered reading is not the
moment that reading describes: an average is centred half a window earlier than
its newest sample. Carrying it forward by its age corrects the transport and
leaves the filter behind. Measured end to end, at 1 m/s:

| | rms error |
|---|---|
| raw, believed to be about now | 0.2414 m |
| checked, calibrated, filtered | 0.0378 m |
| carried forward by its age | 0.0199 m |
| carried forward by the whole lag | 0.0123 m |

for a cost of 0.035 s, of which 0.015 is the filter and 0.020 the transport.

**That is the phase's promise met.** A sensor twenty times better than the one
we started with, and the price written down beside it rather than guessed at by
whoever tunes the loop next.

## Build It

Implement `submit`, `health` and `lag_seconds` in `exercise/solution.hpp`.

```
rcpp verify 15-04
```

The suite checks the channel, then measures what the wrong order costs, then
freezes a sensor two different ways, then runs the whole phase end to end on one
signal.

## Use It

**Give every sensor a channel, and every channel limits from the physics.** The
limits are worth an argument at design time and worth nothing invented in a
hurry after an incident.

**Have callers act on the health, not only on the value.** `no_data`, `stale`
and `stuck` are three different situations and holding position is the right
answer to all three, which is easy to say and only possible if the driver says
which one it is.

**Report `stale` in preference to `stuck`.** A channel that has gone quiet is old
either way, and the age is more useful to a caller than the reason.

**Publish the lag.** A controller tuned against a channel whose lag is a number
can be retuned when that number changes. One tuned against a lag nobody wrote
down will be retuned by trial and error every time the hardware moves.

## What Breaks First

- **A range check placed after the filter.** See `E-SENSE-0011`.
- **A dead sensor that keeps reporting.** See `E-SENSE-0012`.
- **A stuck threshold chosen by habit.** See `E-SENSE-0013`.

## Ship It

`Channel` joins `rc::sensor` beside `Stamped`, and closes phase 15. What began
as a raw number now arrives as a value, a moment, a verdict and a price, and the
four together are what the rest of this curriculum means by a reading.
