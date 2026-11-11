# Ticks on Nice Numbers: Why an Axis Says 0.2 and Not 0.19999999999999998

> An axis is a row of numbers a person reads. Put them on 1, 2 and 5 times a power of ten, compute each from a whole number, and say 0 when it is 0.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 10-02, 01-08

## The Problem

Lesson 10-02 chose the range of a chart. A chart also needs numbers written on its axis, and they have to be at places a person can read: 0, 0.2, 0.4, and not 0.137, 0.274, 0.411. The obvious ways of making them print things nobody should see.

## The Concept

### Floating point on an axis

0.1 added ten times is **0.99999999999999989**, and `3 * 0.1` prints as **0.30000000000000004**. A tick made by adding the step to the last tick drifts, and a label made by printing it shows the drift. See `E-PLOT-0007`.

`Ticks` stores the index of the first tick and the step, and tick k is `(first_index + k) * step` rounded to the decimals the step needs. The ticks for 0 to 1 with about 5 asked for were step 0.2, **6** ticks: 0.0 0.2 0.4 0.6 0.8 1.0.

### The step

`nice_step` takes the raw step, span over target, and rounds it up to 1, 2, 5 or 10 times a power of ten: 1 over 5 is 0.2, 10 over 5 is 2, 7.3 over 5 (1.46) is 2, 100 over 4 (25) is 50, 0.0004 over 5 (8e-5) is 1e-4. 0.5 over 5 is exactly 0.1 and stays 0.1: the comparisons have 1e-9 of slack.

**42** ranges, from -250 to 1e6 and spans from 0.0004 to 1000: **0** ticks outside the range, **0** uneven gaps and **0** with a count outside 2 to 13.

### Minus zero

`-0.004` with two decimals is **-0.00** from plain fixed output, which labels the middle of an axis with a minus sign. `tick_label` gives 0.00, and `-0.006` is still -0.01. Across zero, the ticks for -0.3 to 0.3 were **7**, with exactly **one** labelled 0.0. See `E-PLOT-0008`.

### A steady signal

A range of 5 to 5 has **1** tick, at 5, labelled 5.00, and 5 to 5 + 1e-14, narrower than a double can show at that size, is one tick too. A range of 5 to 5 + 1e-6 has a real step, at least 6 decimals so that the labels show the difference. See `E-PLOT-0009`.

## Build It

`nice_step`, `ticks_for` and `tick_label` in `exercise/solution.hpp`. `Ticks::value` is supplied.

```
rcpp verify 10-05
```

## Use It

`rc/plot/ticks.hpp` joins its module.

```cpp
const rc::plot::Ticks ticks = rc::plot::ticks_for(range.low, range.high, 6);
for (int k = 0; k < ticks.count; ++k) draw_tick(ticks.value(k), rc::plot::tick_label(ticks.value(k), ticks.decimals));
```

**What this does not do.** It places ticks and writes labels. It does not draw them, and it does not know the size of a label on screen: how many fit is the target, chosen by the caller. It has no logarithmic axis, and no dates. The labels are fixed point, so a range of 1e-9 at a centre of 1e6 would need 9 decimals and show them, which is honest and not pretty.

## What Breaks First

- **Ticks made by adding.** See `E-PLOT-0007`.
- **A minus sign on zero.** See `E-PLOT-0008`.
- **No ticks for a steady signal.** See `E-PLOT-0009`.

## Ship It

`rc/plot/ticks.hpp` holds `nice_step`, `ticks_for`, `tick_label` and `Ticks`.

Lesson 10-02 decided how much of the value axis to show. This decides where the numbers go on it, and the next lessons draw them.
