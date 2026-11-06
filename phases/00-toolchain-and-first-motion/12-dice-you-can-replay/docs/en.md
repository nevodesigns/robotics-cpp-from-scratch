# Dice You Can Replay: Seeded Random Numbers, and the Three Ways a Quick Version Is Wrong

> A simulated sensor needs noise, and a test needs to fail the same way twice. Random numbers
> that can be replayed do both, and the quick way to make them is wrong in three separate ways.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-01, 00-04

## The Problem

Lesson after lesson in this curriculum needs noise: a sensor with an error in it, a wheel that slips a little, an input nobody thought of. The noise has to look random, and
when something fails because of it, it has to fail again when you look.

The random numbers that come from the clock or the machine are different every run, so a failure that happened once is gone. And the quick ways of making your own are each wrong in a way that
nothing reports.

## The Concept

### A generator that can be replayed

A **seeded generator** is a rule for getting the next number from the last one. Start it from the same number, the **seed**, and it gives the same sequence on every run and every machine.
xorshift32 is the smallest one worth using: three shifts and three xors.

```cpp
state ^= state << 13;
state ^= state >> 17;
state ^= state << 5;
```

The test checks the first four numbers from seed 1 against a separate calculation: 270369, 67634689, 2647435461 and 307599695.

Seed 12345, the largest of 100 draws was 0.999863 at draw 32, and run again it was 0.999863 at draw 32. A test that prints its seed when it fails can be run again with that seed. See `E-CPP-0082`.

### Zero is not a seed

Zero shifted and xored with itself is zero, so a generator that starts at zero stays there. The constructor replaces a zero seed with a fixed number, and seed 0 gave 1359758873 then 3761132862. See `E-CPP-0081`.

### A number from 0 to n, fairly

The quick way is `next() % n`. It is fair only if n divides the range of `next()`, and otherwise the smaller remainders come up one more time than the rest. With n of 3,000,000,000, the share of
draws below 1,294,967,296 was:

| method | share |
|---|---|
| remainder | 0.604 |
| `below(n)` | 0.433 |
| fair | 0.432 |

For a die the effect is far too small to see, and a six sided die from `below(6)` threw between 99636 and 100523 of each face in 600000 throws. The check is at the large end because that is where it shows. See `E-CPP-0080`.

`below(n)` throws away the few draws that would cause the imbalance: `threshold = (0u - n) % n`, and redraw while the draw is below it.

### Uniform and bell shaped

`uniform()` takes the top 24 bits of a draw and divides by 2^24, which gives a number in [0, 1) that a double holds exactly. Over 200000 draws the mean was 0.4997 and the variance 0.0834, where a half and a
twelfth are right.

`gaussian()` adds twelve of them and takes away 6. Twelve uniform numbers add to a mean of 6 and a variance of 1, so the result has mean 0 and spread 1. Over 400000 draws, mean -0.0009 and spread 0.9997.

It is a cheap bell shape, and the tails are thinner than a true one's: 0.190 percent of draws were beyond three sigma where a true bell curve has 0.270, and the largest of 400000 was 4.35. For noise on a simulated sensor
that is fine, and for a study of rare events it is not.

## Build It

The constructor, `next`, `below`, `uniform` and `gaussian` in `exercise/solution.hpp`.

```
rcpp verify 00-12
```

## Use It

`rc/core/random.hpp` joins `rc::core`.

```cpp
Rng rng(seed);
const double noisy_reading = truth + 0.01 * rng.gaussian();
```

**What this does not do.** xorshift32 is for simulation and tests. It is not for anything secret, because its next number can be worked out from the last. It has a period of 2^32 minus 1, which is plenty for a test and
too few for a very long simulation, and its numbers are only as good as the statistics here show. The standard library has better generators; the point of this one is that every line of it can be read.

## What Breaks First

- **A remainder for a range.** Small values come up too often. See `E-CPP-0080`.
- **A seed of zero.** The generator sticks. See `E-CPP-0081`.
- **A source that cannot be replayed.** A failure that is gone. See `E-CPP-0082`.

## Ship It

`rc/core/random.hpp` holds `Rng`: `next`, `below`, `uniform` and `gaussian`.

Several suites later in this curriculum carry a copy of this class. From here on they could use the one in `librc`.
