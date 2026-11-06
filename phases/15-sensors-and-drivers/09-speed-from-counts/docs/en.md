# Speed From Counts: Differencing an Encoder at Every Speed It Turns

> An encoder gives a position in whole counts and a controller wants a speed. The obvious
> way is wrong at low speed, the better way is late, and the third way fails at high speed.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 15-08, 15-01

## The Problem

Lesson 14-09's inner loop controls speed, and it says the speed has to be measured. An
encoder does not measure speed. It reports a position, in whole counts, and a speed has to
be made from it.

The obvious way is to take this sample's count minus the last one's and divide by the sample
time. At a high speed that is fine. At a low one it is not, and the reason is the same as lesson
15-08's: a count is a whole number, and a sample can only hold a whole number of them.

## The Concept

### One difference

At half a count per sample, a sample holds none or one. The speed it reports is zero or one count
per sample: zero, a thousand, zero, zero, a thousand, with the joint turning smoothly all the time.
It is a hundred percent out.

An encoder of 1000 counts a revolution read at 1 kHz. At 0.05 revolutions a second a count arrives
every 20 samples, and the single difference reads 1000 counts a second for one sample and zero for
the other nineteen. See `E-SENSE-0020`.

### A window

Count the counts over a longer stretch. The resolution is then one count over the whole window, and
at 0.5 revolutions a second a window of 50 samples holds 25 counts and is right to a few percent.

The price is delay, and it is lesson 15-01's. What a window reports is the average speed over it,
which is the speed half a window ago. A speed that doubles at sample 200 reaches the halfway value 24
samples later with a window of 50, half a window, 25 ms.

Until the window has filled it uses the oldest count it has and the number of samples between, and
never divides by a window it has not seen. See `E-SENSE-0021`.

### The time between counts

A different quantity altogether. Stamp each count with the time it arrived, and the speed is one over
the gap between two of them. The resolution is a clock tick, not a count: ten microseconds against
a gap of twenty milliseconds, at 0.05 revolutions a second, is a twentieth of a percent. And it has
no delay beyond a gap.

It fails the other way. At 50 revolutions a second the gap is 20 microseconds and the clock tick is
ten, so a rounding of one tick is half the answer.

It needs three things done right, each of which the test checks.

- **A stopped encoder produces no edges**, so a speed computed from the last two holds the last speed
  for ever. If more than a gap has passed since the last edge, the speed can only be lower, at most one
  count over the time since the last edge. 500 counts a second with a gap of 2 ms reads 10 a tenth of a
  second later and 1 after a second.
- **A reversal is not a speed.** The first edge after turning round is the same position crossed the
  other way. The estimate is zero until a second edge in the new direction, and then signed.
- **It is only to be believed when it is slow.**

See `E-SENSE-0022`.

### Each where it is good

An encoder of 1000 counts a revolution read at 1 kHz, a ten microsecond timer, edges two percent uneven,
as real ones are. The rms error as a percent of the speed:

| speed | counts a second | one sample | window of 50 | edge timing | combined |
|---|---|---|---|---|---|
| 0.05 rev/s | 50 | 435.9 % | 20.0 % | 1.6 % | 1.6 % |
| 0.50 rev/s | 500 | 100.0 % | 2.0 % | 1.6 % | 1.6 % |
| 5.00 rev/s | 5000 | 14.1 % | 0.3 % | 3.9 % | 0.3 % |
| 50.00 rev/s | 50000 | 1.4 % | 0.0 % | 54.1 % | 0.0 % |

No one method is good at every row. A single sample is useless until the speed is high. The window
is good from half a revolution a second and delays by half of itself. Edge timing is good when slow and
fails when fast. Taking the edge timing below 1500 counts a second and the window above it gives the last
column, within 2 percent at every speed.

The threshold is where the two cross: a gap long enough to be many timer ticks, and a window that holds a
good number of counts. A different encoder, clock and sample rate put it somewhere else.

## Build It

`WindowVelocity`, `PeriodVelocity` and `pick_velocity` in `exercise/solution.hpp`. The comments say what
each keeps and what it has to do about the three cases above.

```
rcpp verify 15-09
```

The tests build the encoder in the test from a steady speed, with uneven edges and a rounded clock, so the
table is the same every run.

## Use It

`rc/sensor/velocity.hpp` joins `rc::sensor`.

```cpp
rc::sensor::WindowVelocity window(50, 0.001);
rc::sensor::PeriodVelocity period;
// at each edge, from a timer interrupt
period.on_edge(timer_seconds, direction);
// every sample
const double speed = rc::sensor::pick_velocity(period.estimate(now), window.update(count), 1500.0);
```

**What this does not do.** It assumes a steady speed. A joint that is accelerating hard has a speed that changes
inside the window, so the window's average is not its speed at any moment, and the edge timing is the speed over
one gap, at the time of the previous edge. Both are late, and a lesson on observers does the better thing, which
is to estimate the speed from a model of the motion as well as the measurements. It does not do quadrature
decoding: the direction comes in as a plain argument, and a real encoder's two channels and their glitches are
the front end of this. And the edge timing needs a timer with a resolution and a reliable way to stamp the edge,
which on a microcontroller is an input capture and not a loop that polls.

## What Breaks First

- **One difference.** At a low speed it is zero or one count. See `E-SENSE-0020`.
- **A window that is long enough.** It is late by half of itself. See `E-SENSE-0021`.
- **Edge timing without its three cases.** It holds the last speed, flips on a reversal and fails fast. See
  `E-SENSE-0022`.

## Ship It

`rc/sensor/velocity.hpp` holds `WindowVelocity`, `PeriodVelocity` and `pick_velocity`.

Lesson 15-08 found what a converter throws away. This finds what an encoder does, and the same fact, that a reading
is a whole number of steps, has a different consequence for a speed than it has for a voltage.
