id: E-SENSE-0002
title: A filter long enough to destabilise the loop it feeds
match: too much filtering is also an unstable loop
match: the whole trade, printed
platforms: linux, windows
teaches: 15-01-every-filter-is-a-delay
---

## Symptom

A control loop that was stable becomes oscillatory after somebody smooths the
sensor. The measurement looks better than it ever has. The machine is worse.

Lengthening the filter further, on the theory that the loop is reacting to
noise, makes it much worse.

## Cause

A moving average of N samples delays the signal by half the window, and a delay
in a feedback path is what destroys a loop.

The controller is steering by where the robot **was**, so it keeps commanding
after the error has gone, and the result is overshoot that feeds the next
overshoot.

Measured, on a loop that settles in about 1.2 seconds, with the filter as the
only thing changed:

| window | lag | overshoot | settles |
|---|---|---|---|
| 64 | 0.063 s | 0.3% | 1.18 s |
| 128 | 0.127 s | 2.8% | 1.73 s |
| 256 | 0.255 s | **103%** | never |
| 512 | 0.511 s | **645%** | never |

The failure is not gradual. Between 64 and 256 the overshoot goes from a third
of a percent to over a hundred, and the loop stops settling at all. The filter
that ruins it is the one whose lag is about a fifth of the settling time.

## Fix

Treat the lag as a budget and spend it deliberately.

```cpp
const double lag_seconds = (window - 1) / 2.0 * dt;
```

Keep it small against the loop's own timescale. A twentieth of the settling time
is comfortable, a fifth is not, and the numbers above are one loop rather than a
law, so measure your own.

Then choose the window from what the noise actually requires rather than from
what looks smooth on a chart. Uncorrelated noise falls as one over the square
root of the window, so:

- 4 samples halves the noise and costs 1.5 samples of lag.
- 64 samples divides it by eight and costs 31.5.
- Going from 64 to 256 buys another factor of two and costs four times the lag.

That curve is why long filters are rarely worth it: the noise reduction slows
down and the lag does not.

If the noise genuinely needs more than the loop can afford in lag, the answer is
not a longer average. It is a filter with a better trade for the same delay, or
fixing the sensor, or moving the smoothing outside the loop so the controller
sees the fast signal and only the display sees the smooth one.

The opposite mistake is `E-SENSE-0003`, and both are real: this loop is unstable
with no filter at all as well.
