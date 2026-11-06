# Counts, Not Volts: Quantization and the Noise That Helps

> A converter reports which step the voltage is in, not the voltage. Averaging cannot
> recover what is below a step, unless something moves the input, and then it can.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 15-01

## The Problem

Lesson 15-01 averaged away noise, and lesson 15-02 found what averaging cannot fix. Both
treated a reading as a number that is off by some amount. A reading from a converter is off
in a particular way: it is a whole number of steps, and everything inside one step has been
thrown away.

A common idea is to average a thousand readings to get ten more bits. Sometimes it works
and sometimes it does nothing at all, and what decides which is a thing most people have
never looked at.

## The Concept

### What a converter does

An analogue to digital converter cuts its input range into `2^bits` equal steps and reports
which one the input is in, as a whole number called a code. One step is a **least significant
bit**, an LSB, and it is the range over `2^bits`. Ten bits over 0 to 3.3 V is 1024 steps of
3.2227 mV.

The convention has to be stated, because the other one is the commonest bug. The range is
divided into `2^bits` steps, **not** `2^bits - 1`. The highest code is `2^bits - 1`, and the very
top of the range is the top edge of that last step and not a code of its own. A code stands for
the middle of its step, which makes the error at most half a step either way.

Three bits over zero to eight volts is eight steps of exactly one volt. Dividing by seven makes
a step of 1.143 V, and then the code for 1.0 V is 0 where it should be 1. The test checks the
edges: code 0 at 0.0 and 0.999 V, code 1 at exactly 1.0 V, code 7 at 7.999 V. See `E-SENSE-0017`.

### The ends

A voltage below the range gives the lowest code and a voltage above it gives the highest, and
nothing else. 8 V, 9 V and a thousand volts are all code 7. The converter clips. It does not wrap
and does not report an error, so a reading at either end is ambiguous about how far out the input
was. `saturated` says whether an input is at or beyond an end, which is how code that uses a
reading can tell when it has stopped meaning anything.

### What one reading is wrong by

Never more than half a step. On average, 0.2817 of a step rms, measured over inputs spread across
the range, against the 0.2887 that a step over the square root of twelve predicts, which is the
standard deviation of an error spread evenly over one step.

### Averaging a steady voltage gains nothing

With no noise on the input, a steady voltage gives the same code every time. A thousand readings
are a thousand copies of one number, and their average is that number. There is nothing to cancel.

| readings averaged | rms error, no noise |
|---|---|
| 1 | 0.2817 |
| 100 | 0.2817 |
| 1000 | 0.2817 |

The readout still moves in jumps of one step however many samples go into it. Reporting more digits
than a step is inventing them. See `E-SENSE-0018`.

### Noise makes it work

If something moves the input across the converter's boundaries from reading to reading, the fraction
of readings on each side of a boundary follows where the input really is inside the step, and the
average carries it.

The rms error, in steps, of the average of `n` readings with noise of `sigma` steps added to the input
before the converter:

| noise | n = 1 | n = 100 | n = 1000 |
|---|---|---|---|
| 0.0 | 0.2817 | 0.2817 | 0.2817 |
| 0.3 | 0.4023 | 0.0550 | 0.0389 |
| 1.0 | 1.0489 | 0.1052 | 0.0348 |
| 3.0 | 3.0155 | 0.3094 | 0.1012 |

Noise of 0.3 of a step is already enough to start. About a step is where the error goes as one over
the square root of `n`. More than that it is averaged away, but it starts from further out, and the
single reading is much worse.

### A change smaller than a step

A signal moves by a third of a step. The average of 1000 readings notices it:

| | noticed |
|---|---|
| no noise on the input | 31.17 percent of the time |
| a step of noise | 100.00 percent of the time |

Without noise it is noticed only when the change happens to carry the voltage across a boundary, a
third of the time, and when it does it shows a whole step, three times too much. A quieter sensor is
*worse* at seeing a small change than a noisier one. See `E-SENSE-0019`.

That is dither: noise of about a step added on purpose before the converter, and averaged. It costs the
single reading its accuracy and buys resolution in the average, and it is a trade, not a free gain.

## Build It

`Quantizer` in `exercise/solution.hpp`: `lsb`, `max_code`, `code`, `value` and `saturated`. The comments
state the convention.

```
rcpp verify 15-08
```

The first three tests are the arithmetic and the edges. The last three are the tables above, with noise
added by a random number generator written into the test, so they give the same numbers every run.

## Use It

`rc/sensor/quantizer.hpp` joins `rc::sensor`.

```cpp
const rc::sensor::Quantizer adc(10, 0.0, 3.3);
const long code = adc.code(volts);
if (adc.saturated(volts)) flag_out_of_range();
```

**What this does not do.** It is an ideal converter. A real one has offset and gain errors, which lesson
15-02's calibration finds, and non-linearity: its steps are not equal, so the error is bigger than a step
over the square root of twelve and does not average away. Its own noise may already be larger than a step,
which is why this is not always an issue. And dithering is a method for a signal that is steady while it is
averaged: a signal that changes during the average is smoothed as well, which is lesson 15-01's price.

## What Breaks First

- **A range divided by `2^bits - 1`.** Every reading is a little wrong, more so at the top. See
  `E-SENSE-0017`.
- **Averaging a steady reading for resolution.** There is nothing to cancel. See `E-SENSE-0018`.
- **A small change that is seen sometimes.** It depends where the signal started. See `E-SENSE-0019`.

## Ship It

`rc/sensor/quantizer.hpp` holds `Quantizer`.

Lesson 15-07 removed a spike. This accounts for a reading that is never quite the voltage, and says when
more samples cannot help.
