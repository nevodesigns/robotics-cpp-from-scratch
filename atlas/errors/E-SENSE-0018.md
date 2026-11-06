id: E-SENSE-0018
title: Averaging a steady reading to get more resolution
match: averaging readings of a steady voltage with no noise gains nothing at all
match: expected .*one.* to be within
platforms: linux, windows
teaches: 15-08-counts-not-volts
---

## Symptom

A sensor that reads in whole counts is averaged over a thousand samples to get "ten more bits" and the
result is exactly as coarse as a single reading. The readout still moves in jumps of one count however
many samples go into it. The averaging is working, and the answer has not changed.

## Cause

Averaging reduces noise. A converter with no noise on its input gives the same code every time for a steady
voltage, so a thousand readings are a thousand copies of one number, and their mean is that number. There
is nothing for the average to cancel.

Measured, ten bits over 0 to 3.3 V, so a step of 3.2227 mV, rms error in steps over inputs spread across
the range:

| readings averaged | rms error with no noise |
|---|---|
| 1 | 0.2817 |
| 100 | 0.2817 |
| 1000 | 0.2817 |

A single reading is wrong by about 0.29 of a step, which is a step over the square root of twelve, the
standard deviation of an error uniformly spread over one step. No number of identical readings changes it.

## Fix

Averaging only helps if something moves the input across the converter's boundaries from reading to
reading. That is `E-SENSE-0019`. Without it, the way to more resolution is a better converter, not more
samples. Do not report more digits than the step: a value of 1.23456 volts from a converter whose step is
3.2 mV is four digits of invention.
