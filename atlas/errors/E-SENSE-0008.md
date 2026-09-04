id: E-SENSE-0008
title: A loop destabilised by latency nobody wrote down
match: a loop steering by a late measurement
match: the loop cannot tell where the lag came from
platforms: linux, windows
teaches: 15-03-late-is-wrong
---

## Symptom

A control loop that was well behaved on the bench oscillates on the robot, with
the same gains and the same code. Or it was fine for a year and started
oscillating after somebody swapped the sensor for a nicer one, or moved it onto
a different bus, or added a second device to the same link.

Nothing in the diff touches the controller.

## Cause

The measurement is older than it used to be, and a delay in a feedback path is
what destroys a loop. The controller is steering by where the robot was.

The loop from lesson 14-04, with nothing changed but the age of its measurement:

| measurement age | overshoot | settles | command effort |
|---|---|---|---|
| 0 | 0.0% | 1.21 s | 1.22 |
| 0.030 s | 0.0% | 1.20 s | 1.43 |
| 0.060 s | 0.0% | 1.19 s | 1.69 |
| 0.120 s | 1.3% | 1.49 s | 2.80 |
| 0.240 s | **98.3%** | never | 17.69 |
| 0.500 s | **768%** | never | 19.47 |

Sixty milliseconds is free on this loop. A quarter of a second is fatal to it.
Between those two the failure arrives suddenly rather than gradually.

The second half of the cause is why it is hard to find. Lag from a filter and
lag from a bus do the same damage:

| source | lag | overshoot | settles |
|---|---|---|---|
| a filter of 128 | 0.126 s | 2.8% | 1.74 s |
| a bus 63 steps late | 0.126 s | 3.6% | 1.75 s |
| a filter of 256 | 0.254 s | 103% | never |
| a bus 127 steps late | 0.254 s | 116% | never |

The loop cannot tell them apart, and only one of them is written in your code.
The filter's window is a number somebody chose and can find again. The bus's
latency is in the device's sampling period, its conversion time, its buffer, the
driver's poll rate and the scheduler, and it is written down nowhere.

## Fix

Measure the age rather than reasoning about it.

Stamp a reading when it is taken, carry the stamp with the value, and have the
consumer look at `age_seconds` before acting. If the reading has no timestamp
from the device, stamp it in the driver at the moment of the read: that misses
the device's own internal delay but catches everything after it, which is
usually most of it.

```cpp
const auto reading = sensor.latest();
if (!rc::sensor::fresh(reading, clock.now(), 0.050)) hold();
```

Then add the two lags and compare the total against the loop's own timescale.
A twentieth of the settling time is comfortable, a fifth is not, and both the
filter and the bus spend from the same budget.

If the total is too large, the order to fix things in is: reduce the transport
delay, because it buys nothing; then reduce the filter, because at least it
bought noise reduction; then retune. Retuning first hides the problem behind
gains that will be wrong again as soon as the latency changes.

The related entries are `E-SENSE-0002`, which is the same instability caused by
a filter, and `E-SENSE-0009`, which is the position error the same latency
causes in an estimator rather than a controller.
