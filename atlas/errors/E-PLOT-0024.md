id: E-PLOT-0024
title: Contrast worked out from the raw channel values
match: relative luminance is 0 for black, 1 for white, and weighs green most
match: expected relative_luminance
platforms: linux, windows
teaches: 10-13-the-line-nobody-could-see
---

## Symptom

The contrast of mid grey on white comes out as about 2 and not 4, or a dark blue is called bright, so every threshold is in the wrong place.

## Cause

A stored channel value is not proportional to light. Mid grey 128 is half of 255 and about a fifth of the luminance. Measured: **0.2159**, and the weights are **0.2126**, **0.7152** and **0.0722**.

## Fix

Make each channel linear first:

```cpp
linear = c <= 0.03928 ? c / 12.92 : std::pow((c + 0.055) / 1.055, 2.4);
```
