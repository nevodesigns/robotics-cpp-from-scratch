# The Delay in the Loop: How Late Can the Answer Be Before the Gain Is Too Big

> A delay adds no gain and a great deal of lag. It uses up the margin a loop has
> before feedback turns against itself, and the gain you can have falls as the square
> of how late the answer is.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 14-04, 08-12

## The Problem

Lesson 14-04 chose gains by measurement and found what each costs. A loop tuned that
way on the bench can shake itself apart in the robot with the same gains. The controller
did not change. Something between the sensor and the motor did: a filter, a message over
a link, a sensor that reports a moment after it reads.

None of those makes the loop look bigger. The gain is the same. What a delay adds is
lag, and lag grows with frequency, so it gets worse exactly where a fast loop works.

## The Concept

### A delay is a phase

A delay of `d` seconds shifts a signal at `f` hertz by `360 f d` degrees. Ten
milliseconds at five hertz is 18 degrees, and at fifty hertz it is 180: exactly
backwards. It has no effect on the size of the signal, which is why it is easy to miss.

Every real loop has several, and they add. The sensor reads before it reports. A filter
lags, which phase 15 measures. A message has an age, as in lesson 08-12. And a command
computed at one sample is held until the next, which on average is half a sample late.

### The margin

Take a controller of the kind lesson 14-01 built, proportional and derivative, on a pure
inertia `J`. Its loop gain is `(kp + kd s) / (J s^2)`. Two things matter about it:

- the **crossover frequency**, where the size of the loop gain is exactly one. It has a
  closed form: setting `sqrt(kp^2 + kd^2 w^2) / (J w^2)` to one is a quadratic in `w^2`.
- the **phase margin**, how far the phase there is from minus 180 degrees, past which
  feedback turns positive. The controller leads by `atan2(kd w, kp)`, the inertia lags
  by exactly 180, and the delay lags by `w d`. What is left is

```text
margin = atan2(kd w, kp) - w * d
```

Positive means stable, and a delay only ever subtracts.

Both are checked against the definition. The test evaluates the loop gain as a complex
number, `(kp + j w kd) / (J (j w)^2) * exp(-j w d)`, finds the crossover by searching for
where its size is one, and takes its argument. Across 12 gains and dampings the closed
form was within 4e-16 of the search.

### What a delay does to a gain that was fine

A proportional gain of 3000 with critical damping, on a 0.5 kg m^2 inertia, in a 1 kHz
loop:

| delay | phase margin | the step response |
|---|---|---|
| 0 ms | 71.8 degrees | settles |
| 5 ms | 26.1 degrees | settles |
| 10 ms | -19.6 degrees | grows without bound |
| 20 ms | -110.9 degrees | grows without bound |
| 40 ms | -293.6 degrees | grows without bound |

Five milliseconds takes the margin from 72 degrees to 26. Ten takes it below zero. The
prediction and the time domain simulation agree on which side of zero it is, on every
row. See `E-CTRL-0015`.

### The gain you can have

Flip the question: for a given delay, what is the largest gain at which it is still
stable? The margin falls as the gain rises, so the answer is where it crosses zero, and
it is found by halving the range on a log scale.

The test also finds it the hard way, by simulating a step response with a `DelayLine` in
the loop and halving the gain until it stops settling.

| delay | simulated limit | predicted from the margin | difference |
|---|---|---|---|
| 0 ms | 341737 | 838222 | 145.3 % |
| 5 ms | 6627 | 6928 | 4.5 % |
| 10 ms | 1852 | 1901 | 2.6 % |
| 20 ms | 490 | 499 | 1.8 % |
| 40 ms | 125 | 128 | 1.8 % |

Two things in the table.

**The law.** Each doubling of the delay cuts the limit about fourfold: 3.6, 3.8 and 3.9.
The gain a loop can stand goes as the inverse square of its lateness, so a loop that
is twice as late has a quarter of the gain, and a loop with ten times the delay has a
hundredth.

**The first row.** With no delay the model says there is no limit, and the simulated
loop stops at 341737. Sampling is a delay of its own, and the model of a continuous loop
with a pure delay does not describe it well. The prediction is given half a sample period
added to the delay, which is what holding a command between samples costs, and with a
real delay it is within 4.5 percent. At nothing it is an upper bound and not a figure.
See `E-CTRL-0016`.

### The delay line

`DelayLine` is a ring buffer of `n` samples. Push a value in and the one pushed `n` calls
ago comes out, the initial value until it has filled, and with no samples it is a wire.
Two things go wrong: writing before reading returns the value just stored, which is no
delay at all, and a buffer of zero samples has nothing to index. See `E-CTRL-0017`.

## Build It

`DelayLine`, `phase_lag_degrees`, `crossover_frequency`, `phase_margin_degrees` and
`largest_stable_gain` in `exercise/solution.hpp`. The comments write out the algebra.

```
rcpp verify 14-07
```

The first four tests are the pieces, the last two the tables above. The simulation uses
your `DelayLine`, so a wrong one changes every number.

## Use It

`rc/control/delay.hpp` joins `rc::control`.

```cpp
const double delay = sensor_latency + filter_delay + 0.5 * sample_period;
const double limit = rc::control::largest_stable_gain(inertia, delay);
// keep the gain a good deal below it
```

**What this does not do.** It is one plant, a pure inertia with a PD controller, and the
result that matters is the shape: the gain limit falls as the square of the delay. A real
joint has friction, which helps; a gear train with backlash, which does not; a motor with
its own electrical lag; and a flexible link whose resonance is a second limit. A margin of
45 degrees or more is the usual target, and for a real robot the delay is measured, not
assumed: lesson 07-16 measures the latency of a loop, and lesson 08-12 the age of a message.

## What Breaks First

- **A gain tuned with less delay than the loop has.** The margin goes negative. See
  `E-CTRL-0015`.
- **A limit computed with no delay.** The model has none and the loop does. See
  `E-CTRL-0016`.
- **A delay line off by one, or an angle compared as a quantity.** See `E-CTRL-0017`.

## Ship It

`rc/control/delay.hpp` holds `DelayLine`, `phase_lag_degrees`, `crossover_frequency`,
`phase_margin_degrees`, `margin_at_gain` and `largest_stable_gain`.

Lesson 14-04 found that the gain is a choice with costs. This finds the ceiling on it,
and it is set by something that is not in the controller at all.
