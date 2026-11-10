id: E-MATH-0025
title: Random directions that crowd the poles
match: random latitude and longitude crowd the poles, and choosing the height evenly does not
match: expected this to be true: naive_share
platforms: linux, windows
teaches: 06-13-random-in-every-direction
---

## Symptom

A test throws random directions at a sensor model and the readings from straight up and straight down are fine while those from the sides are poorly covered. A Monte Carlo estimate of how much of the sky is blocked is a few times too high, or too low.

## Cause

Choosing a latitude and a longitude each evenly sounds like choosing a point evenly on a sphere. It is not: every line of latitude is equally likely however short it is, so the tiny circles near the poles get as many points as the equator and the poles are crowded.

Measured, out of 100000 directions, the share within 10 degrees of the north pole: **0.0546** for latitude and longitude, **0.0071** for an even choice, and **0.0076** is the true share of the sphere. The naive draw gave **7.2** times too many.

## Fix

The area of a band of a sphere depends only on its height, so choose the height evenly:

```cpp
const double z = 2.0 * rng.uniform() - 1.0;
const double r = std::sqrt(std::fmax(0.0, 1.0 - z * z));
```
