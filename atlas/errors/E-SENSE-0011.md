id: E-SENSE-0011
title: A range check placed after the filter, which can never fire
match: checking after the filter instead of before it
match: a reading outside the physics is refused, and changes nothing
platforms: linux, windows
teaches: 15-04-the-sensor-that-stopped
---

## Symptom

The driver validates its readings and rejects nothing, ever, while the value it
produces visibly twitches whenever the sensor drops out.

Somebody widens the check, or tightens it, and nothing changes. The rejection
counter stays at zero.

## Cause

The check is downstream of the filter, and the filter has already made the bad
reading plausible.

A dropout reporting zero counts, on a rangefinder calibrated in metres, is
-0.4264 m. That is not a distance and any range check catches it. Averaged over
sixteen samples with fifteen good readings of 5 m it becomes 4.6609 m, which is
a perfectly ordinary distance, and the check that sees only the average has
nothing to object to.

Measured, on one dropout in a steady stream:

| | checked first | filtered first |
|---|---|---|
| refused the dropout | 1 | 0 |
| worst error | 0.0000 m | 0.3391 m |
| outputs spoilt after it | 0 | 15 |

Sixteen outputs carry the bad reading instead of none, and nothing anywhere
reports a problem.

## Fix

Check first, then filter. The order is not a preference.

```cpp
const double calibrated = calibration.apply(raw);
if (!(calibrated >= lowest && calibrated <= highest)) {
  ++rejected;
  return false;      // the filter never sees it
}
value = filter.update(calibrated);
```

A refused reading changes nothing at all: the filter does not see it, the
timestamp is not advanced, and if refusals continue the channel goes stale by
itself, which is the honest thing for it to report.

Two details that go with it.

**Write the test as the requirement, then negate it.** `!(v >= lo && v <= hi)`
rejects a nan; `v < lo || v > hi` accepts one. That is `E-SENSE-0007`.

**Set the limits from the physics, in engineering units.** A room is not four
hundred metres long and a wheel does not turn at ten thousand radians a second.
Limits derived from the ADC's range only reject values the ADC could not have
produced, which is to say none of them.

The order of the other two steps does not matter in the same way: calibrating
and then filtering differs from filtering and then calibrating by 1.8e-15 over
five hundred readings, because a straight line through an average is the average
of a straight line. Calibrate first anyway, so that the range check and
everything downstream of it are in metres.
