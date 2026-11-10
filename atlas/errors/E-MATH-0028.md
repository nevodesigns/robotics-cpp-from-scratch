id: E-MATH-0028
title: Small numbers swallowed by a large running total
match: a thousand ones next to a number ten to the sixteen vanish from the plain total, Kahan keeps them, and pairwise keeps most
match: expected naive to be within
platforms: linux, windows
teaches: 06-14-adding-a-long-list
---

## Symptom

A total of many small readings stops growing, or comes out lower than a hand calculation says. A running distance from odometry, a charge counted in tiny steps and a time accumulated in microseconds each drift from the truth with no error anywhere.

## Cause

A `double` keeps about sixteen digits. Once the running total is large, a small number added to it has its low digits rounded away, and a small enough number is rounded away entirely. Added a million times, a million small pieces are lost.

Measured: `1e16`, then a thousand ones, then `-1e16`. The exact total is **1000**. The plain total was **0.0**: `1e16 + 1` is `1e16` in a double, so every one vanished. Kahan's total was **1000.0**. The pairwise total was **998.0**: it kept most and lost two.

## Fix

Keep what each addition loses, and put it back into the next:

```cpp
const double corrected = x - lost;
const double next = total + corrected;
lost = (next - total) - corrected;
total = next;
```
