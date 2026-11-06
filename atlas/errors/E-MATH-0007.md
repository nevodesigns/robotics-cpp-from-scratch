id: E-MATH-0007
title: Elimination that divides by a small pivot
match: a tiny pivot ruins an elimination that does not swap, and costs a swap nothing
match: a system with a known answer is solved, whatever order its rows come in
platforms: linux, windows
teaches: 06-07-solving-a-system
---

## Symptom

A linear solve that works on every test matrix and returns a wrong answer on one that looks innocent: a perfectly
ordinary two by two system, with nothing nearly singular about it, comes back with an answer that is plainly not a
solution. Reordering the equations changes the answer. Or a solve on a zero in the corner divides by zero and produces
infinity, on a system that has a solution.

## Cause

Gaussian elimination subtracts a multiple of the pivot row from every row below it. The multiple is the entry below over the
pivot, so a small pivot makes a huge multiple, which swamps the rest of the row: the information in the other entries is
rounded away.

Measured, the matrix `[[1e-20, 1], [1, 1]]` with the right hand side `[1, 2]`, whose answer is (1, 1):

| elimination | answer |
|---|---|
| without swapping rows | (0, 1) |
| with the largest entry as the pivot | (1, 1) |

The multiple is 10^20 and the second row becomes `-10^20`, so the 1 and the 2 in it are lost entirely, and the first
unknown comes out as zero. A zero in the corner is the same thing taken to its limit: a division by nothing.

## Fix

Partial pivoting. Before each step, find the row at or below the diagonal with the largest entry in that column, and swap it up:

```cpp
for (std::size_t i = k + 1; i < n; ++i)
  if (std::fabs(a.at(i, k)) > std::fabs(a.at(pivot, k))) pivot = i;
```

Then no multiple is ever bigger than one, and the rounding in each step is the rounding of a number the size of the data. The cost is a
search and a swap. The test also swaps a zero into the corner and checks that it solves a system whose rows are the wrong way round.
