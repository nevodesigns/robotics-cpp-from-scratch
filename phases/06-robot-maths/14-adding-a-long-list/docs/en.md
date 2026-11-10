# Adding a Long List: Why the Total of a Million Numbers Is Wrong, and Two Ways to Get It Right

> An odometer adds a small distance a thousand times a second for hours. Every addition rounds, and what
> each one loses is never put back.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-04, 00-12, 06-06

## The Problem

A robot adds up distances, charges, times and errors, one small number after another, millions of times. Each addition is correct to the last digit a `double` holds, and the total is wrong anyway.

Lesson 00-04 found that numbers lie, and lesson 06-06 found energy leaking from an integrator. This is the plainest form of the same thing: the sum itself.

## The Concept

### Why a total loses things

A `double` keeps about sixteen digits. Add a small number to a large total and the answer is rounded to sixteen digits, so the small number's low digits are gone. If the number is small enough, all of it is.

`1e16`, then a thousand ones, then `-1e16`: the exact total is **1000**. In a double, `1e16 + 1` is `1e16`: the spacing between numbers of that size is 2. The plain total was **0.0**. Every one vanished. See `E-MATH-0028`.

### Keeping what was lost

Kahan's sum keeps `lost`, the part of the last number that did not fit. The next number is corrected by it first:

```cpp
const double corrected = x - lost;
const double next = total + corrected;
lost = (next - total) - corrected;      // exactly what the addition rounded away
total = next;
```

On the example above, Kahan's total was **1000.0**.

### Adding from the middle out

The pairwise sum splits the list in half, totals each half and adds the two. The additions are then between numbers of about the same size. Short pieces, 8 or fewer, are added plainly.

On the example it gave **998.0**: it kept most and lost two. It is not magic, and it costs almost nothing in speed: the same number of additions, in a different order.

### The test needs an exact answer

To measure an error you need a total you know. The suite makes 4 million numbers that are whole multiples of 2^-40, so each is a whole number of units and the exact total is the sum of integers, which a 64 bit integer holds without rounding. Measured against that, with an exact total of 1999858.801753:

| method | relative error |
|---|---|
| plain | 2.53e-14 |
| pairwise | 0 |
| Kahan | 0 |

A reference that is itself a rounded sum is no reference. See `E-MATH-0030`.

### In single precision

A `float` keeps about seven digits, so the effect begins at a much smaller total. A million copies of the float 0.1: the exact total is **100000.00**, the plain total was **100958.34**, **0.96** percent out, and Kahan's was **100000.00**. See `E-MATH-0029`.

## Build It

`sum_kahan`, `sum_pairwise_range` and `sum_kahan_float` in `exercise/solution.hpp`. The plain sums and `sum_pairwise` are given.

```
rcpp verify 06-14
```

## Use It

`rc/math/summation.hpp` joins `rc::math`.

```cpp
const double distance = rc::math::sum_kahan(steps);
```

**What this does not do.** The compensation depends on the compiler doing exactly what is written. A build with `-ffast-math`, which lets the compiler treat addition as exact, can simplify `(next - total) - corrected` to zero and take the compensation out: do not use that flag on this code. It
costs about four additions where the plain sum costs one, which matters in an inner loop and not in an odometer. A running total, where numbers arrive one at a time, needs the running form (keep `total` and `lost` in the object), not the list form. And none of this helps when the numbers themselves are rounded: it keeps the sum faithful to what it was given.

## What Breaks First

- **A large running total.** Small numbers vanish. See `E-MATH-0028`.
- **A `float` accumulator.** A percent out at a million. See `E-MATH-0029`.
- **A reference that is also rounded.** The check passes for the wrong total. See `E-MATH-0030`.

## Ship It

`rc/math/summation.hpp` holds `sum_naive`, `sum_kahan`, `sum_pairwise_range`, `sum_pairwise`, `sum_naive_float` and `sum_kahan_float`.

Lesson 06-06 integrated a motion and found that the error depended on the method. This finds that it also depends on the plain act of adding.
