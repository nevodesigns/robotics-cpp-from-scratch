# Precise and Wrong: What Averaging Cannot Fix

> Averaging a hundred readings and averaging a million gave the same answer, and
> the answer was 27 cm out.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 15-01, 03-02

## The Problem

The last lesson smoothed a noisy sensor and measured what the smoothing cost.
This one is about the error that smoothing never touches.

A rangefinder reads about 1.5 percent short and about 42 cm long. Both at once,
because they are different faults: one is in its scale and one is in its zero.
Neither is random, so neither goes away when you average, and a sensor whose
readings agree beautifully with each other and disagree with the world is the
most convincing wrong answer a robot can be given.

**Precision is the readings agreeing with each other. Accuracy is their agreeing
with the truth.** Filtering buys the first. Only calibration buys the second.

## The Concept

### More samples stop helping, and then what is left is not noise

The sensor is at a true 10 m, jittering by 5 cm:

| samples averaged | estimate | error | noise term |
|---|---|---|---|
| 1 | 10.3132 | 0.3132 | 0.0500 |
| 10 | 10.2883 | 0.2883 | 0.0158 |
| 100 | 10.2722 | 0.2722 | 0.0050 |
| 10 000 | 10.2697 | 0.2697 | 0.0005 |
| 1 000 000 | 10.2700 | 0.2700 | 0.0001 |

The noise column falls by a factor of a thousand. The error column stops moving
after about a hundred samples and sits at 0.27, which is the 1.5 percent it reads
short at 10 m plus the 42 cm it reads long everywhere.

That gives a diagnostic worth keeping. Put the sensor somewhere known and take a
hundred readings, then a thousand:

- **The average keeps improving.** Noise. Filter it, and 15-01 says what that
  costs.
- **The average stops improving and is not the truth.** Bias. No filter will
  ever reach it.

### The correction is not the sensor

There are two straight lines here and confusing them is the most common way to
make a calibrated sensor worse than an uncalibrated one.

```
raw  = 0.985 * true + 0.42     what the sensor does
true = 1.0152 * raw  - 0.4264  what you have to do about it
```

The first is what a datasheet gives you. The second is what goes in your code.
Store the first where the second belongs and every distance comes out about
twice as wrong as doing nothing at all:

| truth | corrected | applied backwards | not corrected |
|---|---|---|---|
| 0 m | 0.000 | +0.834 | +0.420 |
| 5 m | 0.000 | +0.685 | +0.345 |
| 10 m | 0.000 | +0.536 | +0.270 |
| 20 m | 0.000 | +0.238 | +0.120 |

Nothing in that middle column looks alarming. Twenty metres reading as 20.24 is
not a number anybody stops to argue with, which is why this one survives review
and lives in the robot for a year.

### One reference is not enough, and it looks like it is

Zeroing at a single distance is the calibration everybody does first. Here it is
on this sensor, zeroed at 10 m:

| truth | raw | corrected | error |
|---|---|---|---|
| 0 m | 0.420 | 0.150 | **+0.150** |
| 5 m | 5.345 | 5.075 | +0.075 |
| 10 m | 10.270 | 10.000 | 0.000 |
| 15 m | 15.195 | 14.925 | -0.075 |
| 20 m | 20.120 | 19.850 | **-0.150** |

Exact where it was checked. Wrong at both ends, by the same amount, in opposite
directions.

One reference can only ever move the line up and down, and this sensor's line
also has the wrong slope. Two references solve for both, which is the whole of a
straight line and therefore the whole of the correction.

That table is also a diagnosis you can read off the signs, and it is worth
learning:

- **Same error everywhere.** An offset.
- **Opposite errors at the ends, right in the middle.** A scale error.
- **Same-sign errors at both ends, right in the middle.** Not a line at all.

### Fit over the range you will use

Real sensors bend a little somewhere. Here is one with a very slight curve,
calibrated two ways, showing the error at each distance:

| truth | fitted over 0..20 m | fitted over 0..2 m |
|---|---|---|
| 1 m | -0.017 | -0.001 |
| 5 m | -0.067 | +0.014 |
| 10 m | **-0.090** | +0.073 |
| 20 m | 0.000 | **+0.328** |
| 40 m | +0.718 | **+1.386** |

The narrow fit is nearly perfect over the two metres it was made from and then
leaves: 33 cm out at 20 m, 1.39 m at 40. The wide fit is never better than 9 cm
anywhere, and never worse than that inside its range either.

Both are honest about the same fact. **A straight line cannot follow a curve**,
so the question is only which stretch of the curve you want it to be right
about. Choose the stretch the robot works over, and treat everything beyond the
last reference as an estimate.

### More references, better fit

Two points determine a line exactly, so it is tempting to think two references
are enough. They would be, if the references themselves were exact. They are
measured, so they are not.

Least squares over noisy references, rms error of the resulting calibration
across 0 to 20 m, averaged over forty trials:

| references | rms error | of the previous |
|---|---|---|
| 2 | 0.0372 | |
| 8 | 0.0192 | 0.52 |
| 32 | 0.0098 | 0.51 |
| 128 | 0.0052 | 0.53 |

Four times the references, half the error, all the way down. It is the same
square root law the moving average obeyed in 15-01, spent on the references
rather than on the readings, and it is the reason a calibration procedure asks
you to do the tedious thing twenty times.

### Then ask how well it fitted

A fit always succeeds. `rms_residual` says how far the line missed the points it
was made from, and it is the only thing that can tell you the line was the wrong
shape.

Compare it against the sensor's noise rather than against zero, and be honest
about what it can see. The same bend, on the same references, at two noise
levels:

| sensor noise | straight sensor | bent sensor | ratio |
|---|---|---|---|
| 0.0500 | 0.0479 | 0.0530 | **1.11** |
| 0.0050 | 0.0048 | 0.0281 | **5.87** |

At 5 mm of noise the bend is obvious. At 5 cm it is 11 percent, which is less
than the difference between two runs of the same straight sensor.

**The bend is there in both rows.** The residual finds what the noise does not
hide, and a clean residual is evidence that the line is as good as the data, not
that the sensor is straight.

### A fit that cannot be done

The denominator of a least squares slope is the spread of the raw readings, and
it is exactly zero when they are all the same: a reference nobody moved, a sensor
that saturated, a driver returning its last value after the device went quiet.

The numerator is zero too, so dividing gives `0.0 / 0.0`, which is a nan. And a
nan is worse than a wrong number, because it passes the check you wrote:

```cpp
if (value < 0.0 || value > 100.0) reject(value);      // lets the nan through
if (!(value >= 0.0 && value <= 100.0)) reject(value);  // catches it
```

Every comparison against a nan is false, including the ones that would have
caught it. So `from_samples` returns a `rc::expected` and refuses, rather than
handing back a `Calibration` full of nans, and every range check in this
curriculum is written as a requirement that is then negated.

## Build It

Implement the two fits and the residual in `exercise/solution.hpp`.

```
rcpp verify 15-02
```

The suite fits a sensor that is straight, then one that bends, then measures
what averaging does and does not remove, and prints every table above.

## Use It

Three habits, each of which is one line in a commissioning procedure.

**Record the references, not just the result.** A `scale` and an `offset` cannot
be checked by anybody later. The raw readings and the true values can be refitted,
re-examined, and compared against next year's.

**Check the direction with one reading.** Put the sensor somewhere known and far
from zero, and confirm that the corrected value is closer to the truth than the
raw one. That catches a backwards calibration in the workshop.

**Write the residual down beside the calibration**, along with the sensor's
noise. The pair of numbers is what makes the fit auditable; either alone says
nothing.

## What Breaks First

- **Averaging a bias and expecting it to converge.** See `E-SENSE-0004`.
- **A one-point calibration hiding a scale error.** See `E-SENSE-0005`.
- **A calibration applied in the wrong direction.** See `E-SENSE-0006`.
- **A degenerate fit producing a nan the range check misses.** See
  `E-SENSE-0007`.

## Ship It

`Calibration` joins `rc::core` beside the filters. From here every sensor in this
curriculum is read through both: the calibration removes what averaging cannot,
and the filter removes what calibration cannot, and neither one substitutes for
the other.
