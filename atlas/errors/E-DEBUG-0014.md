id: E-DEBUG-0014
title: The comment said one thing and the code did another
match: (the comment says|the comment said|comment and the code disagree|comment is wrong)
platforms: linux, windows
teaches: 01-18-the-comment-said-otherwise
---

## Symptom

A change is made on the strength of a comment and the behaviour is not what the
comment promised. Or a function is called by what its name and comment say, and
the numbers it returns are plausible and wrong. Nothing crashed, nothing warned.

Often it is found weeks later, by someone who traced a value by hand because it
looked a little off.

## Cause

A comment is a claim. The code is the fact. Nothing checks that the two agree,
and code is changed far more often than the comment above it.

Measured on five functions from a sensor path, 25 cases, comparing what each
comment claims with what the code returns:

| function | comment wrong in |
|---|---|
| `smooth`, "moves 20 percent towards it" | 5 of 6 |
| `deadband`, "passes through unchanged" | 3 of 5 |
| `last_mean`, "or of all of them if fewer" | 2 of 5 |
| `percent`, "0 to 100" | 2 of 5 |
| `is_stale`, "as old as the limit or older" | **0 of 4** |

12 of 25 wrong, and the last row exactly right. Reading code is not distrusting
it. It is finding out which comment is the wrong one, and the only way to find out
is to trace an input and see.

## Fix

Predict before you run.

1. Read the comment and write down what it claims.
2. Pick the smallest input that exercises the line you doubt, and trace it by
   hand with the actual numbers.
3. Write your prediction. Then run it. A wrong prediction marks the exact line
   you misread.
4. Pin the current answers in a test before changing anything.
5. Change as little as you can, and run the pin again.

When you find a comment that is wrong, fix the comment in the same change. The next
reader will trust it.
