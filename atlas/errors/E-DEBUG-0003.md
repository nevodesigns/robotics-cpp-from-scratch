id: E-DEBUG-0003
title: Sections that do not add up to the loop
match: a whole iteration, broken down
platforms: linux, windows
teaches: 04-05-which-part-is-slow
---

## Symptom

A control loop takes a millisecond and the four parts you measured come to
eight hundred microseconds. The shares add to eighty eight percent. Somebody
asks where the rest went and nobody knows.

Or the reverse, and worse: the shares add to a hundred and seventy percent,
and the report is quietly nonsense.

## Cause

Two different problems with the same symptom, and the sign of the remainder
tells you which.

**A positive remainder is the code you did not name.** Measured on a loop
broken into four parts:

| section | total | share |
|---|---|---|
| read sensors | 250 us | 25% |
| estimate | 400 us | 40% |
| control | 80 us | 8% |
| write motors | 150 us | 15% |
| **unnamed** | **120 us** | **12%** |

The 120 microseconds is the copying between the stages, the logging, the bounds
check, the thing added last month. It is unnamed because nobody thought it was
worth a section, which is exactly why it is where the surprise lives.

**A negative remainder means the sections overlap.** An outer section
containing an inner one measures wall time, so it includes the inner. Measured:
outer 700 us, inner 500 us, accounted for 1200 us, of a loop that took 700.

```
unaccounted_for reports -500 us
```

That is the whole nested section, counted twice.

## Fix

**Measure the whole iteration as well as the parts, and print the
difference.** `rc::core::SectionTimer::unaccounted_for` is one subtraction and
it is the only number in the report that tells you about the code you did not
instrument.

**Do not clamp it at zero.** A negative value is the only signal that the
sections overlap. Clamping turns a detectable mistake into a report that is
merely wrong.

**Keep sections flat.** One level, covering disjoint stretches of the
iteration. Nesting is sometimes what you want, and when it is, do not add the
shares up.

**Name the gap once you find it.** A twelve percent remainder is not a mystery
to live with; it is a section waiting to be added. Add it, and the remainder
goes to zero or reveals the next one.

**Suspect the remainder before you optimise the biggest section.** Forty
percent in `estimate` is visible and probably known. Twelve percent that nobody
can account for is where the unexpected thing is.
