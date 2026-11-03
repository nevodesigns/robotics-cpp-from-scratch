# Close Enough: Choosing a Tolerance That Means Something

> The same tolerance of 1e-9 covers four and a half million distinct doubles at
> magnitude 1, and fewer than one at magnitude 1e9.

**Type:** Build
**Time:** about 105 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 00-04, 01-04

## The Problem

Lesson 00-04 told you never to compare two computed doubles for exact equality,
and to ask whether they are close enough instead. That advice is correct and it
is half a sentence. Close enough to what, measured how, and does the answer
still hold when the numbers get bigger?

This lesson is the other half, because choosing the tolerance is where the real
bugs are. A tolerance chosen by eye is loose in one part of your program and
mathematically impossible in another, and nothing tells you which is which.

## The Concept

### Doubles are not spread evenly

This is the fact everything else follows from. Doubles are dense near zero and
sparse out at range, so the distance to the next representable value depends on
where you are:

| magnitude | gap to next | can it notice `v + 1.0` |
|---|---|---|
| 1e0 | 2.220e-16 | yes |
| 1e3 | 1.137e-13 | yes |
| 1e6 | 1.164e-10 | yes |
| 1e9 | 1.192e-07 | yes |
| 1e12 | 1.221e-04 | yes |
| 1e15 | 1.250e-01 | yes |
| 1e16 | 2.000e+00 | **no** |
| 1e17 | 1.600e+01 | **no** |

Read the last two rows again. At 1e16 the gap between neighbouring doubles is
2.0, so **adding one to that number does nothing whatever**. Not rounds, not
loses a little. The result is bit for bit what you started with.

### Which is why one absolute tolerance means two things

Now take a tolerance of 1e-9, which looks small and looks like one thing, and
ask what it actually buys at each magnitude:

| magnitude | gap to next | doubles 1e-9 spans | adjacent pair within it |
|---|---|---|---|
| 1e0 | 2.220e-16 | 4.504e+06 | yes |
| 1e3 | 1.137e-13 | 8.796e+03 | yes |
| 1e6 | 1.164e-10 | 8.590e+00 | yes |
| 1e9 | 1.192e-07 | 8.389e-03 | **no, nothing can** |
| 1e12 | 1.221e-04 | 8.192e-06 | **no, nothing can** |

Both ends are wrong, in opposite directions.

**At magnitude 1 it is loose.** Four and a half million distinct doubles satisfy
it. If you chose 1e-9 because it felt strict, it is not.

**At magnitude 1e9 it is impossible.** It cannot reach the next representable
double. There is nothing between a value and its neighbour, so no two
*different* numbers out there are ever within 1e-9 of each other. An algorithm
that iterates until two values agree to 1e-9 can only ever stop by landing on
the identical bit pattern.

That distinction matters more than it sounds. A tolerance that is merely strict
makes an algorithm slow. A tolerance below the gap makes it unsatisfiable, and
those two failures look identical from outside.

### A relative tolerance means the same thing everywhere

Scale the tolerance by the size of the numbers and the problem goes away:

| magnitude | doubles it spans | adjacent pair equal |
|---|---|---|
| 1e0 | 4.504e+06 | yes |
| 1e3 | 8.796e+06 | yes |
| 1e6 | 8.590e+06 | yes |
| 1e9 | 8.389e+06 | yes |
| 1e12 | 8.192e+06 | yes |

Between 4.5 and 8.8 million doubles at every magnitude from 1 to 1e12. A factor
of two across twelve orders of magnitude, against a factor of 500 million for
the absolute version.

### And it still needs an absolute floor, for zero

A relative tolerance compares against the size of the values, and near zero
there is no size to compare against:

```
1e-18 vs 2e-18, relative only:    NOT equal
0.0   vs 1e-300, relative only:   NOT equal
```

Those two values differ by a factor of two, so relatively they are a hundred
percent apart, and nothing is ever close to zero.

So the comparison takes both and passes if either does. Relative alone cannot
reach zero; absolute alone cannot reach 1e9. Neither is the answer on its own,
which is why `nearly_equal` has four parameters and not three.

### Accumulation, where this stops being theoretical

A robot steps a clock forward. The obvious loop:

```cpp
double t = 0.0;
while (t != 10.0) { step(); t += 0.1; }
```

**This never terminates.** Not rarely, never. A hundred additions of 0.1 reach
`9.99999999999998046`, which is not 10.0, and the loop runs past it forever.

Write `t < 10.0` instead and it does terminate, after **101 steps**, because
after a hundred steps the clock is still just under ten. That extra sample is
usually the one that lands outside the trajectory.

And the drift is not a fixed cost:

| steps | error | doubles adrift |
|---|---|---|
| 10 | 1.110e-16 | 1 |
| 1000 | 1.407e-12 | 99 |
| 100000 | 1.885e-08 | 10000 |
| 1000000 | 1.333e-06 | 92000 |

About a tenth of a representable double per step, forever. The fix is simpler
than the bug: keep the step count as an integer and multiply. `100 * 0.1` is
exactly 10.0.

### Summation, where the order changes the answer

One value of 1e16 followed by a hundred thousand values of 1.0:

| order | sum | error |
|---|---|---|
| largest first | 10000000000000000.0 | **-100000.0** |
| smallest first | 10000000000100000.0 | 0.0 |
| compensated, largest first | 10000000000100000.0 | 0.0 |

Adding the big one first **discards all one hundred thousand** of the small
values, because 1.0 is below the gap at 1e16. The sum is exactly the first
value.

This is the clearest demonstration that floating point addition is not
associative, and therefore that **a sum is not a property of a set of numbers.
It is a property of a set of numbers and a traversal order.**

Two repairs, both exact, by different routes. Sorting smallest first lets the
small values accumulate into something large enough to survive meeting the big
one. Compensated summation keeps whatever did not fit and feeds it back on the
next addition, which works when the values arrive one at a time and cannot be
sorted.

### One honest note about this repository

`rc::test::nearly_equal`, behind `RC_CHECK_NEAR`, takes a single absolute
tolerance. By everything above, that is the wrong shape.

It is the right shape for a test. There, the expected magnitude is written on
the line next to the tolerance, so a reader can check the number by eye, and
`RC_CHECK_NEAR(volts, 1.65, 1e-6)` says exactly what it means.

It is the wrong shape for a comparison in a running system, where the magnitude
is whatever the robot happens to be doing. Same operation, different job,
different tool.

## Build It

Implement `gap_above`, `ulps_between`, `nearly_equal` and `kahan_sum` in
`exercise/solution.hpp`.

```
rcpp verify 01-08
```

The suite prints `0.1 + 0.2` to twenty places, walks the gap between doubles
across eight magnitudes, reads one tolerance both ways across five, steps a
clock until it does and does not arrive, and adds a hundred thousand ones to
1e16 three different ways.

## Use It

**Scale your tolerance, and keep a floor.** `rc::core::nearly_equal(a, b,
relative, absolute)`. The relative one does the work; the absolute one is for
zero.

**Read a tolerance in representable doubles before trusting it.**
`ulps_between` turns "is 1e-9 reasonable here" into a number. Under one means
impossible, millions means loose.

**Never wait for a float to equal anything.** Use `>=` for a deadline, or
`nearly_equal` when you genuinely mean the same value.

**Derive time from an integer step count.** `step * dt`, not `t += dt`. Exact,
and shorter.

**Integrate the delta, not the absolute.** An odometer that accumulates
distance travelled and adds the starting position once at the end never has a
large total to lose increments into.

**Never build this with `-ffast-math`.** It permits the compiler to reassociate
`(next - total) - adjusted` to zero, which silently reduces `kahan_sum` to
plain addition.

## What Breaks First

- **A tolerance loose at one magnitude and impossible at another.** See `E-NUM-0020`.
- **A clock that never reaches its deadline.** See `E-NUM-0021`.
- **Small values added to a large total and discarded.** See `E-NUM-0022`.

## Ship It

`gap_above`, `ulps_between`, `nearly_equal` and `kahan_sum` join `rc::core`.
Every convergence test, deadline and integrator in the rest of this curriculum
is written with them, which is why none of them waits for a float to be exactly
anything.
