id: E-MATH-0009
title: Least squares by the normal equations, which squares the problem
match: fitting a polynomial: squaring the problem costs twice the digits
match: forming A transpose A squares the condition number, which is where the digits go
platforms: linux, windows
teaches: 06-07-solving-a-system
---

## Symptom

A curve fit, a calibration or a state estimate that works for a low order model and falls apart when the order is raised a little, long before
the data runs out. The fit residual is fine and the coefficients are nonsense. A library fit gives sensible numbers on the same data.

## Cause

The textbook way to the least squares answer is to form the normal equations, `A^T A x = A^T b`, and solve them. It is correct and it is the
obvious method, and it squares the condition number: `A^T A` is a much worse matrix than `A`, and the answer loses about twice as many digits as
it should.

Measured, the sixth Hilbert matrix has a condition number of 2.91e+07 and the matrix of its normal equations 4.59e+14, which is 0.54 of the square.

A polynomial fitted to a hundred exact samples on 0 to 1, with known coefficients, the largest error in a coefficient:

| degree | normal equations | QR |
|---|---|---|
| 3 | 1.96e-12 | 6.27e-15 |
| 6 | 1.31e-08 | 4.57e-13 |
| 9 | 6.27e-06 | 6.91e-11 |
| 12 | 1.00e+00 | 2.09e-08 |

QR is better at every degree and by more the harder the problem is: at degree 9 by a factor of 10^5, and at degree 12 the normal equations have no
digits left and QR still has eight.

## Fix

Do not form `A^T A`. Reduce `A` to upper triangular form by Householder reflections, apply the same reflections to `b`, and back substitute. No
product of `A` with its transpose is ever formed, so the condition number is `A`'s own. The sign of each reflection is chosen to avoid cancellation in
the first entry. And for a matrix with dependent columns, or fewer rows than columns, there is no unique answer and the routine says so.

In practice use a library for this, which does it well, and use this lesson to know what its numbers mean and when to distrust them.
