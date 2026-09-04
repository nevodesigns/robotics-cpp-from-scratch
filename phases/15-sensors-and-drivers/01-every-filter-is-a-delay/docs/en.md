# Every Filter Is a Delay: What Smoothing Costs the Loop

> The measurement looks much better. The machine is much worse. Both of those
> are the filter.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 14-04, 01-04

## The Problem

A sensor is noisy. Every reading jitters by a centimetre, and the obvious answer
is to average a few of them.

It works: the chart from lesson 10-02 goes from a fuzzy band to a clean line, and
it is genuinely more accurate. So somebody lengthens the filter, because if a
little is good then more is better, and the loop that was steady begins to
oscillate.

That is the whole lesson. **A filter is not a free improvement, it is a trade**,
and both ends of it are measurable, so this lesson measures them rather than
offering a rule of thumb.

## The Concept

### What a window buys, and what it costs

Both sides have a formula, and both are worth confirming rather than trusting:

| window | noise left | predicted | lag (samples) | predicted |
|---|---|---|---|---|
| 1 | 1.003 | 1.000 | 0 | 0.0 |
| 4 | 0.498 | 0.500 | 1 | 1.5 |
| 16 | 0.246 | 0.250 | 7 | 7.5 |
| 64 | 0.121 | 0.125 | 31 | 31.5 |
| 256 | 0.058 | 0.062 | 127 | 127.5 |

Uncorrelated noise falls as **one over the square root** of the window. The
delay is the average age of what is in it, **half the window**.

Read those two columns together and the design decision falls out. Going from 4
to 16 divides the noise by two and costs six more samples of delay. Going from
64 to 256 also divides the noise by two, and costs ninety six. The noise
reduction slows down as a square root and the lag grows linearly, which is why
long averages are rarely worth what they cost.

### The delay is the part that matters

A filter in a feedback path means the controller is steering by where the robot
**was**. It keeps commanding after the error has gone, and that is overshoot.

Measured, on the loop from lesson 14-04, changing nothing but the filter:

| window | lag | overshoot | settles | command effort |
|---|---|---|---|---|
| 1 | 0.000 s | 9.2% | never | 17.97 |
| 4 | 0.003 s | 1.5% | 1.11 s | 12.22 |
| 16 | 0.015 s | 0.3% | 1.21 s | 3.93 |
| **64** | **0.063 s** | **0.3%** | **1.18 s** | **2.48** |
| 128 | 0.127 s | 2.8% | 1.73 s | 3.45 |
| 256 | 0.255 s | **103%** | never | 16.86 |
| 512 | 0.511 s | **645%** | never | 17.90 |

Two things in that table are worth more than the rest.

**It is a U, not a slope.** Too little filtering is as bad as too much. With no
filter the loop never settles either, and it spends seven times the command
effort doing it, because the derivative term differentiates the noise: a
millimetre of jitter between samples two milliseconds apart is half a metre per
second, and the controller acts on that as though the robot had lurched.

**The bad end arrives suddenly.** Between 64 and 256 the overshoot goes from
0.3 percent to 103, and the loop stops settling at all. There is no gentle
warning. The filter that ruins this loop is the one whose lag is about a fifth
of its settling time.

So the rule is a budget rather than a number:

```cpp
const double lag_seconds = (window - 1) / 2.0 * dt;
```

Keep that small against the loop's own timescale. A twentieth is comfortable, a
fifth is not, and the table above is one loop rather than a law, so measure
yours.

### The first N readings

A moving average has to answer before it has a full window, and there is a right
answer and a tempting one.

```cpp
return total / static_cast<double>(filled_);   // right
return total / static_cast<double>(window_);   // the warm up transient
```

Dividing by the window counts the empty slots as zeros, so the first reading
comes out as one Nth of the truth and climbs over N samples. At 500 Hz with a
window of 64 that is an eighth of a second of a sensor reading low: short enough
to miss in testing, long enough for a robot parked against a wall to believe the
wall is far away and start driving at it.

Divide by what is there. With one sample, the best estimate is that sample.

### What not to worry about

The filter here recomputes its sum over the window, which is N additions per
reading rather than two. Carrying a running sum is the obvious improvement and
raises an obvious worry: does the rounding accumulate?

Measured, over twenty million samples, a carried sum and a recomputed one differ
by **7.5e-10**. It does not accumulate in any way that matters for doubles.

The reason to write the simple one here is that it cannot be wrong, not that the
fast one is dangerous. When a measurement says the additions matter, carry the
sum.

## Build It

Implement `MovingAverage::update` and the two predictions in
`exercise/solution.hpp`.

```
rcpp verify 15-01
```

The suite checks the filter, then measures both sides of the trade against the
formulas, then puts it inside the controller from phase 14 and prints the table
above.

## Use It

Every sensor from here is read through one of these. Two habits go with it.

**Filter for the consumer, not in general.** The controller and the display want
different things: the display can afford a quarter of a second of lag and the
loop cannot. Two filters on one sensor is not waste, it is two different
questions being answered correctly.

**Write the lag down next to the window.** A window of 64 means nothing on its
own; 63 milliseconds against a loop that settles in 1.2 seconds is a decision
somebody can check.

## What Breaks First

- **A filter that lies for its first N readings.** See `E-SENSE-0001`.
- **A filter long enough to destabilise the loop.** See `E-SENSE-0002`.
- **A derivative term differentiating the noise.** See `E-SENSE-0003`.

## Ship It

`MovingAverage` joins `rc::core::filters`, beside the array versions from lesson
01-04. Those answer a question about data you already have; this one answers it
about data still arriving, which is the situation a robot is actually in.
