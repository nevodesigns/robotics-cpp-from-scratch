id: E-SENSE-0009
title: A reading timestamped when it arrived rather than when it was taken
match: the error a late reading causes is speed times latency, exactly
platforms: linux, windows
teaches: 15-03-late-is-wrong
---

## Symptom

The robot's idea of where it is lags behind where it is, by an amount that grows
with speed. It parks accurately, creeps accurately, and overshoots its stopping
point when it is moving.

Every static test passes. The error appears the first time it drives at speed
and is blamed on the controller.

## Cause

The reading was labelled with the time it reached your code, and it describes a
moment earlier than that. Everything downstream then places it at the wrong
point on the robot's path.

The error is exactly speed times latency. With a sensor 20 ms behind:

| speed | error |
|---|---|
| 0.0 m/s | 0.0000 m |
| 0.1 m/s | 0.0020 m |
| 0.5 m/s | 0.0100 m |
| 1.0 m/s | 0.0200 m |
| 4.0 m/s | 0.0800 m |

**Zero standing still.** That single row is why this survives: a stationary
bench test cannot see it, however carefully it is done, and neither can a slow
one. It scales with exactly the thing you turn up last.

Twenty milliseconds is not a pessimistic figure. A device sampling at 50 Hz has
20 ms of latency before anything else happens, and the conversion, the buffer,
the bus and the poll rate are all still to come.

## Fix

Carry the moment with the value.

```cpp
rc::sensor::Stamped<double> reading;
reading.value = raw;
reading.sampled_at = when_the_device_sampled;
reading.valid = true;
```

Then move it to now, at whatever rate you believe:

```cpp
const auto here = rc::sensor::carried_forward(reading, speed_estimate, clock.now());
```

That removes the whole error when the rate is right, and most of it when it is
not: what is left is the original error times how wrong the rate was, so an
estimate 10 percent out leaves 2 mm of the 20.

Use the device's own timestamp when it has one. When it does not, stamp in the
driver at the moment of the read and write down what that misses, which is the
device's internal sampling and conversion. A latency you have written down and
partly corrected is a different thing from one nobody has ever measured.

To find out whether you have this problem: drive a straight line at two speeds
and compare the position error. An error that doubles when the speed doubles is
this, and no amount of controller tuning will touch it.
