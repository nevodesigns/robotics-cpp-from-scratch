# Inputs You Did Not Think Of: Properties, Generators and Shrinking

> Nine careful examples found nothing. The machine found it in 1.6 values, and
> a different machine, aimed slightly differently, would never have found it at
> all.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 05-03, 01-05

## The Problem

Lesson 05-03 wrote the test that catches a specific bug, and proved it catches
it. That is the right way to fix a fault you already have.

This is about the ones you do not have yet. Examples come from the same head
that wrote the code, and that head has already decided what the interesting
cases are.

## The Concept

### What nine careful cases miss

A parser for unsigned decimal numbers, with no overflow check. Here are the
cases a careful person writes:

```
"0"  "1"  "42"  "999"  "1000000"  ""  "x"  "12x"  "007"
```

Empty, non-numeric, trailing rubbish, leading zeros, a large round number. Every
category somebody would think to list.

**None of them fail.** The parser passes all nine while silently wrapping around
on any number of twenty digits or more, reporting success and a value that is
not the one it was given.

The input that catches it is `18446744073709551616`. There is nothing clever
about it. It is longer than anybody types into a test by hand.

### A property instead of an example

```cpp
// Not: parse("42") == 42
// But: if the parser accepted it, the value prints back as the text.
bool reads_what_was_written(const std::string& text) {
  const auto parsed = parse(text);
  return !parsed.ok || std::to_string(parsed.value) == without_leading_zeros(text);
}
```

Now the machine picks the inputs. It found the fault in **1.6 values on
average**, across twenty seeds.

Choosing the property is most of the work, and it pays to choose a strong one. A
weaker property here, that the value survives being printed and read again, is
**true even when the parser has wrapped**, because the wrapped value is a
perfectly good number and round trips as itself. "It read the number you wrote"
is the statement with content in it.

Keep the examples too. They document intent, they run in microseconds, and they
are how a reader learns the interface. What they are not is a search.

### The generator decides what is reachable

The same fault, hunted three ways. A hundred thousand values per seed, twenty
seeds:

| generator | found it in | mean values |
|---|---|---|
| any printable byte | 0 of 20 | |
| digits, up to 40 | **20 of 20** | **1.6** |
| digits, up to 10 | 0 of 20 | |

Two different failures, at the two ends.

**Too wide.** Over all printable bytes, twenty digits in a row essentially never
happens. It is not that the generator cannot get there; it will not, in any run
you have time for.

**Too narrow.** Limited to nine digits it **cannot** get there. The largest
number nine digits can spell is nowhere near the largest the parser can hold, so
no value it can produce will ever overflow. That generator reports the parser
correct for ever, at any number of trials, on any machine.

From the outside those two are the same thing: a green tick and a big number.

So a passing property test is a statement about the generator's reach as much as
about the code, and the reach is worth checking directly, because it is
arithmetic rather than statistics:

```cpp
for (int length = 1; length <= 9; ++length)
  RC_CHECK(property(std::string(length, '9')));   // nothing this short overflows
RC_CHECK(!property("99999999999999999999"));      // and this is the shortest that does
```

Two lines, and they turn "we ran two million values" into "no value that
generator can produce would ever have found this".

### Shrinking is what makes it usable

The value that fails is whatever the generator happened to produce, and most of
it is irrelevant. Six failures, before and after:

| as found | shrunk |
|---|---|
| 215856221693831863882 | 21111111111111111111 |
| 18837838843399351949413936882156082 | 51111111111111111111 |
| 391834549387509500786351405958096482 | 51111111111111111111 |
| 4351229706018950011832575193254165886 | 51111111111111111111 |
| 82196778937208922990 | 81111111111111111111 |
| 89658864623096167551735378 | 61111111111111111111 |

Mean length **29.17 before, exactly 20.00 after**. Twenty digits is the shortest
number this parser gets wrong, and the shrinker found that without being told.

The algorithm is short: propose candidates that are shorter first and simpler
second, take the first that still fails, and repeat until nothing proposed
fails. The candidates know nothing about why the value failed; the property
decides.

The one thing a shrinker must never do is hand back a value that passes. Every
value it returns has to have failed on the way.

A shrunk counterexample is usually enough to see the bug without a debugger,
which is the difference between a tool people use and one they turn off.

### A failure you cannot repeat is not evidence

The generator is seeded and the seed is reported.

```cpp
rc::test::Source source(seed);
```

A generator seeded from the clock finds a bug, prints a failure, and takes the
evidence with it. The next run is a different run, so the fault becomes "the
suite is flaky", which is worse than not having the test.

Use a fixed seed in the suite, so the same values run every time and a change in
the result means a change in the code. Use a rotating seed in a longer nightly
job, and record which one it used.

### And prove the property can fail

A property written carelessly is satisfied by the code doing nothing:

```cpp
const auto parsed = parse(text);
if (parsed.ok) return true;       // says nothing about anything accepted
return parsed.value == 0;
```

That passes on the broken parser and on the fixed one, at any number of trials.
The tell is exactly that: it does not change when the code does.

The way to find out is the way lesson 05-03 established for any test. Break the
code on purpose, watch the property go red, then fix it and watch it go green. A
test nobody has seen fail is a test nobody has evidence about.

## Build It

Implement `Source::below`, `shrink` and `hunt` in `exercise/solution.hpp`.

```
rcpp verify 05-04
```

The suite checks that the same seed gives the same values, runs nine hand
written cases against a broken parser, hunts the fault with three generators,
shrinks what it finds, and then demonstrates a property that cannot fail.

## Use It

**Write properties for anything that parses, encodes, or converts.** Those are
where inputs come from outside and where examples run out first.

**Say what the generator can produce**, in a comment beside it, in the same terms
as the code's edge cases. "Digits, up to forty, so it reaches past the twenty
that overflow" is checkable. "Random strings" is not.

**Report the seed and the trial count with every result.** One trial in two
means the generator is aimed well; a hundred thousand and nothing means one of
the two failures above, and it matters which.

**Use more than one generator.** One aimed at the structure, one deliberately
hostile, one drawn from recorded input. They fail differently, which is why you
want several.

## What Breaks First

- **A suite written by the person who wrote the code.** See `E-TEST-0005`.
- **A generator that cannot reach the fault.** See `E-TEST-0006`.
- **A property that cannot fail, or a failure that cannot be repeated.** See
  `E-TEST-0007`.

## Ship It

`Source`, `hunt`, `shrink` and `simpler_strings` join `rc::test` beside the test
framework and the leak checker. Every parser and every converter from here can
be held to a property rather than to a list, and the phase's promise stands: a
suite you can prove catches bugs, because you watched it catch them.
