id: E-DEBUG-0002
title: It compiles, it runs, and the answer is wrong
match: driving straight does not change the heading
match: spinning in place does not change the position
match: driving there and back returns the robot
platforms: linux, windows
teaches: 00-06-when-the-answer-is-wrong
---

## Symptom

The build is clean. The program runs to completion. The number at the end is
wrong, and there is no message, no caret and no line number, because nothing
went wrong as far as the machine is concerned.

Rereading the code does not help, and it does not help for a specific reason:
the mistake is invisible because you already believe the code is right.

## Cause

There is no single cause, which is why this entry is about method rather than
about a fix. What there is instead is a small set of shapes, and the shape of
the wrong answer names the region to search:

| what you see | what it usually is |
|---|---|
| exactly zero, always | integer division |
| out by a factor of two | something averaged, or applied twice |
| out by a factor of ten or a thousand | a unit: degrees for radians, millimetres for metres |
| the sign is flipped | a subtraction the wrong way round |
| correct at the start, drifting later | an error that accumulates each step |
| correct for small inputs, wrong for large | a range, or an overflow |
| out by about 1.57 or 3.14 | radians, a quarter or a half turn out |
| correct except at the boundary | a comparison that should be `<=` |

## Fix

Three moves, in this order, and each is cheaper than the one before it is worth
skipping.

**Ask what must be true.** Not what the code does: what the answer has to
satisfy whatever the code does. A robot with both wheels at equal speed cannot
change heading. A rotation matrix cannot change the length of a vector. A filter
cannot report more confidence than it was given. Every property that holds
removes a region from suspicion without a line being read.

**Halve the input.** Five hundred steps are wrong: is one? If one step is right
and five hundred are wrong, the error accumulates and the geometry is innocent.
If one step is already wrong, five hundred steps of arithmetic have become one,
and one can be checked by hand. The smallest input that still fails is the
cheapest thing you will ever debug.

**Print at the boundary before printing in the middle.** The inputs and the
result, first. Half the time the input was already wrong and nothing between
them mattered. Then print the value you are *not* suspicious of, because that is
where it will be.

Two traps to rule out before believing any of it, because both report a bug that
is not there:

- Comparing fractional numbers for exact equality. See `E-NUM-0003`.
- Subtracting two headings, when angles wrap. See `E-NUM-0006`.
