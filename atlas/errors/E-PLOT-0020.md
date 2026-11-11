id: E-PLOT-0020
title: A wheel zoom that does not undo itself
match: a wheel step of 1.12 in and 0.88 out does not come back, and 1.2 to the power of the notches does
match: expected good.t1
platforms: linux, windows
teaches: 10-09-the-zoom-that-moved-the-point
---

## Symptom

Zooming in a few notches and back out leaves the chart a different size from where it started, and doing it again makes it drift further.

## Cause

A step of 1.12 in and 0.88 out are not reciprocals: 1.12 times 0.88 is 0.9856. Measured: five notches in and five out left a width of **107.522** from 100; with 1.2 to the power of the notches they left exactly **100**.

## Fix

Make the factor a power, so that a turn out is exactly the reciprocal of a turn in:

```cpp
return std::pow(1.2, static_cast<double>(angle_delta) / 120.0);
```
