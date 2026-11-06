# Solving a System: Pivoting, Conditioning, and Why You Do Not Square a Problem

> Elimination is easy to write and easy to get wrong in ways that return a confident answer.
> Know what pivoting is for, what a condition number says, and why least squares is not
> solved the way the textbook writes it.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 06-04

## The Problem

A robot's maths is full of linear systems. A calibration solves for a handful of parameters. A fit
finds the coefficients of a curve. A filter solves for a gain. The rest of this curriculum uses a library
to do it, and a library is a thing that returns a number.

Some of those numbers are wrong, and nothing in the return says so. A solve can return a perfectly ordinary
looking answer, with no error, to a problem whose answer it cannot know. This lesson writes the solver, so
that what its numbers mean is no longer a mystery.

## The Concept

### Elimination, and the pivot

To solve `A x = b`, subtract from each row below the first a multiple of the first row, chosen to make the entry in
the first column zero. Repeat down the diagonal. What is left is upper triangular, and is solved from the bottom up.

The multiple is the entry below over the **pivot**, the entry on the diagonal. A small pivot makes a huge multiple,
which swamps the rest of the row: its information is rounded away.

The matrix `[[1e-20, 1], [1, 1]]` with the right hand side `[1, 2]`, whose answer is (1, 1):

| elimination | answer |
|---|---|
| without swapping rows | (0, 1) |
| with the largest entry as the pivot | (1, 1) |

The multiple is 10^20, the 1 and the 2 in the second row are lost, and the first unknown comes out zero. A zero in
the pivot position is the same thing at its limit, a division by nothing.

**Partial pivoting** is the cure. Before each step, find the row at or below the diagonal with the largest entry in
that column and swap it up. No multiple is then ever bigger than one. The test solves systems of sizes 2 to 12 with known
answers to 1e-12, and a system whose rows are the wrong way round. See `E-MATH-0007`.

### Singular and nearly singular

A matrix like `[[1, 2], [2, 4]]` has no unique solution, since its second row is twice its first. The solver finds a pivot
that is zero to the rounding of the largest entry and says so, and returns `ok = false`, not a number.

A matrix like `[[1, 2], [2, 4.000001]]` is not singular and is solved. `ok` does not mean the answer can be trusted. For that,
there is another number.

### The condition number

Some problems cannot tell nearby answers apart. The **condition number** of a matrix is how much it magnifies relative error, and
10^k means about k of the digits in the answer are gone before any solver has done anything. It is a property of the problem, not of
the method, and pivoting does not change it.

It is the 1-norm of the matrix times the 1-norm of its inverse. `condition_1norm` finds the inverse by solving for each column of the
identity, which is affordable for a small matrix and not how a library does it. The fourth Hilbert matrix has a published condition
number of 28375, and the test agrees.

The Hilbert matrix, with entries `1 / (i + j + 1)`, solved against a right hand side made from the answer (1, 1, ..., 1):

| n | condition number | largest error | digits lost |
|---|---|---|---|
| 4 | 2.84e+04 | 6.11e-13 | 3.8 |
| 6 | 2.91e+07 | 5.26e-10 | 6.7 |
| 8 | 3.39e+10 | 4.19e-07 | 9.6 |
| 10 | 3.54e+13 | 3.18e-04 | 12.5 |
| 12 | singular to working precision | | |

Each step of two in `n` costs about three digits, and at every size the error is below the condition number times 10^-15, the
rule of thumb. At n = 12 a pivot falls below the rounding of the largest entry, and the solver declines; the true condition number
is about 10^16, which is all the digits a double has. See `E-MATH-0008`.

### Least squares, and the answer that costs twice as much

More equations than unknowns have no exact solution, and the least squares answer is the x that makes `A x` closest to `b`. The
textbook way is to form the normal equations, `A^T A x = A^T b`, and solve them. It is correct, and it **squares the condition
number**: `A^T A` is a much worse matrix than `A`.

The sixth Hilbert matrix has a condition number of 2.91e+07. The matrix of its normal equations has 4.59e+14, which is 0.54 of the
square.

A polynomial fitted to a hundred exact samples on 0 to 1, whose coefficients are known. The largest error in a coefficient:

| degree | normal equations | QR |
|---|---|---|
| 3 | 1.96e-12 | 6.27e-15 |
| 6 | 1.31e-08 | 4.57e-13 |
| 9 | 6.27e-06 | 6.91e-11 |
| 12 | **1.00** | **2.09e-08** |

QR is better at every degree, by a factor of 10^5 at degree 9, and at degree 12 the normal equations have no digits left and QR has eight.

QR reduces `A` to upper triangular form by reflections, applies the same reflections to `b`, and back substitutes. No product of `A` with
its transpose is formed, so the condition number is `A`'s own. See `E-MATH-0009`.

## Build It

`solve`, `condition_1norm`, `least_squares_normal` and `least_squares_qr` in `exercise/solution.hpp`. `Matrix` and its products are given.
The comments write out the algorithms, including the sign choice in the reflection.

```
rcpp verify 06-07
```

The tests give every method a problem whose answer is known, so the error is a measurement and not an opinion, and the pivoting baseline,
without the swap, is written into the test.

## Use It

`rc/math/linear.hpp` joins `rc::math`.

```cpp
bool ok = false;
const std::vector<double> x = rc::math::solve(a, b, ok);
if (!ok || rc::math::condition_1norm(a) > 1e10) distrust(x);
const std::vector<double> fit = rc::math::least_squares_qr(design, samples, ok);
```

In practice use a library for this: it is faster, it handles the large cases and it has a better condition estimate. Use this lesson to know what
its numbers mean and when to distrust them.

**What this does not do.** It is dense and cubic, so it is for matrices of tens and not thousands. It has no scaling, which is the first thing to
try on an ill conditioned problem: columns of wildly different sizes make a worse condition number than the problem has. It does not use column
pivoting, so it will not find the rank of a nearly rank deficient matrix, which a pivoted QR or the singular value decomposition does. And the
condition number it computes costs as much as the inverse, where an estimate costs a few solves.

## What Breaks First

- **Elimination with no swap.** A small pivot swamps the row. See `E-MATH-0007`.
- **A solve trusted because it returned.** The condition number says how many digits there are. See `E-MATH-0008`.
- **Least squares by the normal equations.** It squares the problem. See `E-MATH-0009`.

## Ship It

`rc/math/linear.hpp` holds `Matrix`, `multiply`, `largest_entry`, `solve`, `one_norm`, `condition_1norm`, `least_squares_normal` and
`least_squares_qr`.

Phase 06 now has vectors, rotations, quaternions, frames, derivatives, integration and a solver. The estimation of phase 16 is a sequence of exactly
these systems, and the filter of 16-02 chose a gain from a small one.
