# A Sensor You Can Trust: Calibrate, Reject, Smooth, and Measure What It Cost

> Every filter on this sensor was added because it fixed something. This is where you find out what each one took in return.

**Type:** Capstone
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 15-07, 15-01, 15-02, 08-01

## The Problem

This phase gave a sensor reading a calibration to take the bias out (15-02), a rejection filter for the readings that are wildly wrong (15-07) and a smoother for the noise (15-01), each with a cost the lesson measured on its own.
Put together in a driver they interact: the order matters, and the costs add up. A smoother that sits before the rejection filter smears one wild reading over several samples, where the rejection filter, placed first, would have removed it.

This lesson builds the pipeline and measures what it cost with four numbers, instead of finding out from a controller that has become sluggish.

## The Concept

### Order

```cpp
const double calibrated = config_.calibration.apply(raw);
const double clean = config_.use_hampel ? hampel_.update(calibrated) : calibrated;
return smoother_.update(clean);
```

Calibrate first, so that every later stage works in real units, and the rejection filter's `min_scale` means something in them. Reject next, while an outlier is still one reading. Smooth last. A raw reading of 3.0 through a scale of 2 and an offset of 1 came out as 7.0000. See `E-SENSE-0035`.

### Four costs

**Noise left.** The rms of the output over the rms of the input on a steady signal with noise of standard deviation 1. A smoother at alpha lets through the square root of alpha over 2 minus alpha:

| alpha | noise left | predicted |
|---|---|---|
| 1.00 | 0.939 | 1.000 |
| 0.50 | 0.557 | 0.577 |
| 0.20 | 0.339 | 0.333 |
| 0.10 | 0.241 | 0.229 |
| 0.05 | 0.173 | 0.160 |

At alpha 1 the smoother does nothing, and the rejection filter alone took off 6 percent of the noise, by replacing the few readings it took for outliers. See `E-SENSE-0037`.

**Lag on a ramp.** A ramp of one unit per second comes out late by (1 - alpha) / alpha samples:

| alpha | lag (s) | predicted (s) |
|---|---|---|
| 1.00 | 0.0000 | 0.0000 |
| 0.50 | 0.0100 | 0.0100 |
| 0.20 | 0.0400 | 0.0400 |
| 0.10 | 0.0900 | 0.0900 |
| 0.05 | 0.1900 | 0.1900 |

That is lesson 15-01's result, and lesson 14-12 is where a delay like it meets a control loop. See `E-SENSE-0037`.

**What an outlier leaks.** One reading in a hundred was 50 against a true zero. The output peaked at 0.000 of the spike with the rejection filter first, and 0.200 without it, which is alpha: the smoother lets a share of the spike in. See `E-SENSE-0035`.

**Time to follow a step.** A real change, not an outlier, looks like one at first, and a rejection filter holds it back until enough of its window agrees. Time to half way after a step from 0 to 1:

| window | alpha | with rejection (s) | without (s) |
|---|---|---|---|
| 3 | 0.50 | 0.01 | 0.00 |
| 3 | 0.20 | 0.04 | 0.03 |
| 7 | 0.50 | 0.03 | 0.00 |
| 7 | 0.20 | 0.06 | 0.03 |
| 15 | 0.50 | 0.07 | 0.00 |
| 15 | 0.20 | 0.10 | 0.03 |

The rejection filter holds a step back by about half its window, and the smoother's delay is added on top. A wide window rejects better and follows worse. See `E-SENSE-0036`.

### Choosing

| alpha | noise left | ramp lag (s) | step to half (s) | spike leak |
|---|---|---|---|---|
| 1.00 | 0.939 | 0.0000 | 0.03 | 0.000 |
| 0.50 | 0.557 | 0.0100 | 0.03 | 0.000 |
| 0.20 | 0.339 | 0.0400 | 0.06 | 0.000 |
| 0.05 | 0.173 | 0.1900 | 0.16 | 0.000 |

A quieter output lags more, and no row is best: the right one is the one a controller can live with, which is a question about the loop's own budget (lesson 14-12). The point of the table is that it exists. See `E-SENSE-0037`.

## Build It

`Pipeline::update` and the four measurement functions in `exercise/solution.hpp`: `measure_noise_ratio`, `measure_ramp_lag`, `measure_step_time` and `measure_leak`. The configuration, the noise generator and the filters from earlier lessons are given.

```
rcpp verify 15-16
```

Everything is deterministic: the noise is from a generator seeded in the code, so the figures above are the same on every run.

## Use It

`rc/sensor/pipeline.hpp` joins `rc::sensor`.

```cpp
PipelineConfig config;
config.calibration = calibration;   // from lesson 15-02
config.alpha = 0.2;
Pipeline sensor(config);
const double trusted = sensor.update(raw_reading);
const double lag = measure_ramp_lag(config, 0.01);   // and put it in the loop's delay budget
```

**What this does not do.** It measures with signals made for the purpose: a steady signal with noise, a ramp, a step and a spike. A real sensor's noise is not always Gaussian or white, and its outliers are not always single. It does not look at timestamps, so a late reading is
smoothed as if it were on time (lesson 15-03), and it does not say when the sensor has gone quiet (lesson 15-04). The figures are for 100 Hz and a window of 7 unless stated.

## What Breaks First

- **Smoothing before rejecting.** A spike leaks a share of itself, alpha, into the output. See `E-SENSE-0035`.
- **A wide rejection window.** It rejects well and holds real steps back. See `E-SENSE-0036`.
- **Filtering with no measurement.** The loop is slower than anyone knew. See `E-SENSE-0037`.

## Ship It

`rc/sensor/pipeline.hpp` holds `PipelineConfig`, `Pipeline` and `measure_noise_ratio`, `measure_ramp_lag`, `measure_step_time` and `measure_leak`.

This ends phase 15: a noisy sensor made usable, with the cost of making it so measured, and a camera frame read the way it was actually laid out.
