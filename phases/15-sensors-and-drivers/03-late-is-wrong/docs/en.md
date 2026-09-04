# Late Is Wrong: The Reading and When It Was Taken

> The loop could not tell whether the lag came from a filter somebody chose or
> from a bus nobody measured. Only one of those two numbers is written down.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 15-01, 03-05

## The Problem

The first two lessons of this phase dealt with a sensor's value: what is random
in it, and what is not. This one is about the other half of a reading, which is
usually thrown away.

A number arrives from a device. Your code uses it. In doing so it makes a claim
nobody wrote and nobody checked: **that the number is about now**.

It never is. The device sampled, converted, buffered, waited its turn on a bus,
and was eventually copied into your process, and none of that appears anywhere
in your source. The gap is tens of milliseconds on ordinary hardware, and what
it costs you is exactly speed times latency, which is zero when you are standing
still.

That last part is why this is hard. It is invisible on a bench.

## The Concept

### A late measurement destabilises a loop

Take the loop from 14-04, which settles in about 1.2 seconds, and change nothing
except how old its measurement is:

| late | seconds | overshoot | settles | command effort |
|---|---|---|---|---|
| 0 | 0.000 | 0.0% | 1.21 | 1.22 |
| 5 | 0.010 | 0.0% | 1.20 | 1.28 |
| 15 | 0.030 | 0.0% | 1.20 | 1.43 |
| 30 | 0.060 | 0.0% | 1.19 | 1.69 |
| 60 | 0.120 | 1.3% | 1.49 | 2.80 |
| 120 | 0.240 | **98.3%** | never | 17.69 |
| 250 | 0.500 | **768%** | never | 19.47 |

Sixty milliseconds is free. A quarter of a second is fatal. The turn is sudden,
and it happens at about a fifth of the settling time, which is exactly where the
filter lag turned in lesson 15-01.

### The loop cannot tell where the lag came from

That is not a coincidence, and this is the measurement that matters most in the
lesson. The same lag, produced two different ways:

| source | lag | overshoot | settles |
|---|---|---|---|
| a filter of 64 | 0.062 s | 0.0% | 1.19 |
| a bus 31 steps late | 0.062 s | 0.0% | 1.19 |
| a filter of 128 | 0.126 s | 2.8% | 1.74 |
| a bus 63 steps late | 0.126 s | 3.6% | 1.75 |
| a filter of 256 | 0.254 s | 103% | never |
| a bus 127 steps late | 0.254 s | 116% | never |

Same lag, same verdict. The filter is marginally gentler, because it attenuates
as well as delays, and the difference is nothing beside the agreement.

So **there is one lag budget, and two things spend from it**. The difference
between them is not what they do to the loop, it is what you can see:

- The filter's window is a number in your code. You chose it, you can find it,
  and lesson 15-01 gave you the formula for what it costs.
- The bus's latency is spread across the device's sampling period, its
  conversion, its buffer, the driver's poll rate and the scheduler. It is
  written down nowhere, and it changes when somebody swaps a cable.

This is why a loop that was fine for a year starts oscillating after a hardware
change that touched nothing in the controller.

### The error is speed times latency, and zero at rest

Away from control loops, in the estimator, the same latency shows up as a
position that trails the truth. With a sensor 20 ms behind:

| speed | worst error | speed x 0.020 s |
|---|---|---|
| 0.00 m/s | 0.0000 | 0.0000 |
| 0.10 m/s | 0.0020 | 0.0020 |
| 0.50 m/s | 0.0100 | 0.0100 |
| 1.00 m/s | 0.0200 | 0.0200 |
| 2.00 m/s | 0.0400 | 0.0400 |
| 4.00 m/s | 0.0800 | 0.0800 |

Exactly, to the last digit, at every speed. There is nothing statistical about
it and nothing to tune.

**Read the first row again.** A stationary test cannot detect this, however
carefully it is done. Neither can a slow one. It scales with the thing you turn
up last, after everything else has been signed off.

### Jitter, and why smoothing makes it worse

Real latency is not constant. A reading is 4 ms old sometimes and 36 ms old at
others, depending on where the sample fell relative to the poll and what else
was using the bus.

At 1 m/s, driving in a straight line:

| | rms | worst |
|---|---|---|
| believed to be about now | 0.0222 m | 0.0360 m |
| and then smoothed over 16 | 0.0350 m | |
| carried forward instead | 0.0000 m | 0.0000 m |

The middle row is the one to stop at. Smoothing is what everybody reaches for
when a reading scatters, and here it **makes the error worse by more than half**,
because a moving average of 16 adds its own 15 ms of lag to the 20 the bus had
already cost.

The error was never zero mean noise on the value. It is the value being about
the wrong moment, and averaging a set of wrong moments gives you an older wrong
moment.

### Carrying a reading forward

The fix is to keep the moment, and then move the reading to now.

```cpp
rc::sensor::Stamped<double> reading;
reading.value = raw;
reading.sampled_at = when_the_device_sampled;
reading.valid = true;

const auto here = rc::sensor::carried_forward(reading, speed_estimate, clock.now());
```

The rate is itself an estimate, and it does not have to be a good one. What is
left after the correction is the original error times how wrong the rate was:

| rate estimate | error left | of the 2 cm |
|---|---|---|
| exact | 0.0000 | 0% |
| 10% out | 0.0020 | 10% |
| 25% out | 0.0050 | 25% |
| 50% out | 0.0100 | 50% |
| 100% out | 0.0200 | 100% |

The bottom row is not compensating at all, which is where you started. Every row
above it is better than that, so there is no threshold below which this stops
being worth doing.

### Freshness, phrased so a nan fails it

A reading also has to be recent enough to act on, and that check is written the
way lesson 15-02 insisted:

```cpp
return age >= 0.0 && age <= max_age_seconds;
```

The requirement, and the caller acts only when it holds. Phrased the other way,
as a list of ages to reject, a nan limit would pass every reading rather than
none, because every comparison against a nan is false.

A negative age is refused as well, and is not a hypothetical: a device with its
own oscillator will hand you a timestamp from the future the day its clock and
yours disagree. `age_seconds` reports it rather than clamping it, because a
clock problem is worth seeing.

## Build It

Implement `age_seconds`, `fresh` and `carried_forward` in
`exercise/solution.hpp`.

```
rcpp verify 15-03
```

The suite checks the three of them, then puts a late measurement inside the
controller from phase 14, then measures what latency costs an estimator, and
prints every table above.

## Use It

**Stamp at the source.** Use the device's own timestamp when it has one. When it
does not, stamp in the driver at the moment of the read, and write down what
that misses, which is the device's sampling and conversion. A latency partly
measured is a different thing from one never measured.

**Add the two lags before tuning.** The filter and the transport spend from one
budget. If the total is too large, shorten the transport first, because it buys
nothing at all; then the filter, which at least bought noise reduction; and
retune last. Retuning first hides the problem behind gains that will be wrong
again the next time the hardware changes.

**Test at speed.** A latency bug and a correct system are the same system at
rest. Drive a straight line at two speeds and compare: an error that doubles
when the speed doubles is this, and no gain will touch it.

## What Breaks First

- **A loop destabilised by latency nobody wrote down.** See `E-SENSE-0008`.
- **A reading timestamped when it arrived.** See `E-SENSE-0009`.
- **Latency that jitters, mistaken for sensor noise.** See `E-SENSE-0010`.

## Ship It

`Stamped` opens `rc::sensor`, the module the rest of this phase builds in. From
here a reading in this curriculum is a value, a moment, and whether it arrived
at all, and the three travel together, because every question worth asking about
a sensor needs more than the number.
