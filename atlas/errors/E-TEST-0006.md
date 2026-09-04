id: E-TEST-0006
title: A generator that cannot reach the fault
match: the generator decides whether the fault is reachable
platforms: linux, windows
teaches: 05-04-inputs-you-did-not-think-of
---

## Symptom

A property test runs millions of values and never fails, and the code is wrong
anyway. Or it runs millions of values, never fails, and nobody can say whether
that means anything.

## Cause

Generated testing searches wherever the generator points, and a passing run is a
statement about the generator's reach as much as about the code.

The same fault, an unsigned parser with no overflow check, hunted three ways.
A hundred thousand values per seed, twenty seeds:

| generator | found it in | mean values |
|---|---|---|
| any printable byte | 0 of 20 | |
| digits, up to 40 | **20 of 20** | **1.6** |
| digits, up to 10 | 0 of 20 | |

Two different failures at the two ends.

**Too wide.** A generator over all printable bytes almost never emits twenty
digits in a row, so the interesting region has effectively zero probability.
It is not that it cannot get there; it is that it will not, in any run you have
time for.

**Too narrow.** A generator limited to nine digits **cannot** get there at all,
because the largest number nine digits can spell is nowhere near the largest the
parser can hold. That one would report the parser correct for ever, at any
number of trials, on any machine.

The two look identical from the outside: a green tick and a large number of
trials.

## Fix

**Aim the generator at the shape of the input**, not at the type. The input here
is a decimal number, so generate decimal numbers; the interesting lengths are
around where the type overflows, so generate lengths that reach past it.

**Say what the generator can produce**, in a comment next to it, in the same
terms the code's edge cases are in. "Digits, up to forty of them, so it reaches
past the twenty that overflow" is a sentence somebody can check. "Random
strings" is not.

**Measure how hard it worked.** Report the number of values tried before the
failure. One in two means the generator is aimed straight at the fault; a
hundred thousand and nothing means one of the two failures above, and it is
worth knowing which.

**Check reachability directly where you can.** It is arithmetic, not
statistics:

```cpp
for (int length = 1; length <= 9; ++length)
  RC_CHECK(property(std::string(length, '9')));   // nothing this short can overflow
RC_CHECK(!property("99999999999999999999"));      // and this is the shortest that can
```

Two lines, and they turn "we ran two million values" into "no value that
generator can produce would ever have found this".

**And use several generators.** One aimed at the structure, one deliberately
hostile, one drawn from real recorded input. They fail differently, which is the
point of having more than one.
