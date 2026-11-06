id: E-MATH-0008
title: A solve that returns an answer, and the answer is wrong in the first digit
match: a solver loses as many digits as the condition number says, and no pivoting can help
match: expected this to be true: error_at_10
platforms: linux, windows
teaches: 06-07-solving-a-system
---

## Symptom

A solve that returns without complaint and a result that is off by a lot: a calibration whose parameters change in the fourth
digit when one input is changed in the twelfth, a fit that swings wildly with a tiny change in the data, a result that changes with
the compiler or the optimisation flag. Every check that the solve did not fail passes.

## Cause

Some problems cannot tell nearby answers apart. The condition number of a matrix is how much it magnifies relative error, and a
condition number of 10^k means about k digits are gone before any solver has done anything: a property of the problem, not of the method.
Pivoting does not help it.

Measured, the Hilbert matrix, whose entries are `1 / (i + j + 1)`, solved against a right hand side made from the answer (1, 1, ..., 1):

| n | condition number | largest error | digits lost |
|---|---|---|---|
| 4 | 2.84e+04 | 6.11e-13 | 3.8 |
| 6 | 2.91e+07 | 5.26e-10 | 6.7 |
| 8 | 3.39e+10 | 4.19e-07 | 9.6 |
| 10 | 3.54e+13 | 3.18e-04 | 12.5 |
| 12 | singular to working precision | | |

Each step of two in n costs about three more digits, and the error stays below the condition number times 10^-15 at every
size. At n = 12 the solver finds a pivot below the rounding of the largest entry and declines. The condition number there is about
10^16, which is all the digits a double has.

A matrix that is nearly singular is solved, and `ok` says nothing about whether the answer can be trusted. A matrix that is exactly
singular, `[[1, 2], [2, 4]]`, is reported.

## Fix

Compute the condition number and look at it. `condition_1norm` finds the inverse a column at a time and multiplies the two 1-norms, which is
affordable for a small matrix. If it is above about 10^10, do not trust the answer to more than a few digits, and change the problem: a better
parameterisation, scaled variables, or more or different data. The solver cannot fix a question that has no sharp answer.
