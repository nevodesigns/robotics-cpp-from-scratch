id: E-TEST-0015
title: What coverage cannot see
match: every line ran and it still broke
platforms: linux, windows
teaches: 05-07-the-line-nobody-ran
---

## Symptom

Coverage is complete, the assertions are real, and a defect still reaches the
machine. Nothing in the report suggests where to look, because by its own
measure there is nothing left to cover.

## Cause

Coverage is a measure of **lines**, and several whole categories of defect are
not properties of a line.

**The input you never thought of.** Every line of a function can run while the
one value that breaks it is never passed. A clamp that mishandles exactly zero
is fully covered by a test using 1 and -1. Lesson 05-04 is the answer: state a
property and let generated inputs look for the counter-example.

**The order you never ran.** Lesson 05-06 measured two probes sharing one
`double` scoring 1 of 2 forwards and 2 of 2 backwards. Both orders execute the
identical lines, so coverage is the same for a suite that is order dependent
and one that is not.

**The interleaving you never hit.** Two threads touching one value cover the
same lines whether or not they collide. The thread sanitizer is the tool, and
it cannot be combined with the address sanitizer (`E-DEBUG-0010`).

**The defect no tool reports.** An uninitialised read is covered, sanitized by
nothing, and measured in lesson 02-09 as passing every compiler and the
address sanitizer in the case that reaches a robot.

**The line that should exist and does not.** This is the largest blind spot
and the hardest to act on. Coverage can only report on code that was written.
A missing bounds check, a missing timeout, a case the author never considered:
none of these appear in any report, because there is no line to be uncovered.

## Fix

Treat coverage as one instrument among several, and know what each is for.

| instrument | finds |
| --- | --- |
| coverage | lines nobody ran |
| assertions | results nobody checked, once the line runs |
| property tests, 05-04 | inputs nobody thought of |
| running the suite backwards, 05-06 | orders nobody tried |
| sanitizers, 04-07 | three of five common defect classes |
| breaking it on purpose, 05-03 | whether the suite would notice |
| a review, and a specification | the line that was never written |

The last row is the one with no tool in it, and it is where the remaining
defects live once the others are in place.

A practical sequence that works: get the never-run list to something you can
read and justify line by line, then stop looking at the percentage and spend
the effort on the rows below it. A suite at 85 percent whose uncovered lines
you can each name and dismiss is in better shape than one at 98 percent that
nobody has read.
