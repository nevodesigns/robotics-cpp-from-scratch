id: E-SENSE-0004
title: Averaging more readings will not remove a bias
match: averaging drives the noise to nothing and leaves the bias untouched
platforms: linux, windows
teaches: 15-02-precise-and-wrong
---

## Symptom

A sensor is noisy, so somebody averages it. The readings settle down beautifully
and are still wrong, by roughly the same amount they were wrong before.

The response is to average harder. Ten readings, then a hundred, then a
thousand, then a filter so long the loop downstream misbehaves, and the number
does not move.

## Cause

Averaging removes the part of the error that is random. It does nothing at all
to the part that is not.

A rangefinder reading `0.985 x + 0.42` with 5 cm of jitter, sitting at a true
10 m:

| samples averaged | estimate | error | noise term |
|---|---|---|---|
| 1 | 10.3132 | 0.3132 | 0.0500 |
| 10 | 10.2883 | 0.2883 | 0.0158 |
| 100 | 10.2722 | 0.2722 | 0.0050 |
| 10 000 | 10.2697 | 0.2697 | 0.0005 |
| 1 000 000 | 10.2700 | 0.2700 | 0.0001 |

A million readings. The noise is down to a tenth of a millimetre, and the answer
is 27 cm wrong: the 1.5 percent it reads short at 10 m, plus the 42 cm it reads
long everywhere. Past about a hundred samples nothing further happens, because
there is nothing left to average away.

Averaging a hundred and averaging a million are the same answer. If more samples
stop helping, what is left is not noise.

## Fix

Measure the sensor against something you trust and correct it.

```cpp
const auto fit = rc::core::Calibration::from_samples(raw, truth);
if (!fit) return rc::unexpected<CalibrationError>(fit.error());
const double distance = fit->apply(sensor.read());
```

Two references are enough for a straight sensor, and more is better than two
because the references have their own error, which the fit averages down by the
same square root law: four times the references, half the error.

The diagnostic that separates the two problems takes a minute. Put the sensor at
a known distance and take a hundred readings.

- **They scatter around the truth.** Noise. Filter it, and lesson 15-01 says
  what that costs.
- **They scatter around something else.** Bias. No filter will ever fix it, and
  calibration will.

Look at where the error sits at more than one distance before deciding which
correction you need: an error the same size everywhere is an offset, and one
that grows with the reading is a scale error. Correcting the wrong one is
`E-SENSE-0005`.
