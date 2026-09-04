id: E-TEST-0007
title: A property that cannot fail, and a failure that cannot be repeated
match: a property that cannot fail proves nothing
match: a source asked twice for the same seed gives the same values
match: shrinking is what makes the failure readable
platforms: linux, windows
teaches: 05-04-inputs-you-did-not-think-of
---

## Symptom

Three complaints that come from the same place.

A property test has never failed in its life and nobody trusts it. A property
test failed once on the build server and nobody could reproduce it. A property
test failed and the input is thirty six characters of noise that nobody can read.

## Cause

**A property that is vacuously true.** Written carelessly, a property can be
satisfied by the code doing nothing at all:

```cpp
const auto parsed = parse(text);
if (parsed.ok) return true;       // says nothing about anything accepted
return parsed.value == 0;
```

That passes on the broken parser and on the fixed one, at any number of trials.
The tell is exactly that: it does not change when the code does.

**A source with no seed.** A generator seeded from the clock finds a bug, prints
a failure, and takes the evidence with it. The next run is a different run. That
is worse than no test, because it teaches people that the suite is flaky.

**No shrinking.** The value that fails is whatever the generator happened to
produce, and most of it is irrelevant. Six failures from an aimed generator,
before and after:

| as found | shrunk |
|---|---|
| 215856221693831863882 | 21111111111111111111 |
| 18837838843399351949413936882156082 | 51111111111111111111 |
| 391834549387509500786351405958096482 | 51111111111111111111 |
| 89658864623096167551735378 | 61111111111111111111 |

Mean length 29.17 before, exactly 20.00 after. Twenty digits is the shortest
number that parser gets wrong, and every shrunk value says so plainly.

## Fix

**Break the code on purpose and watch the property fail.** The same discipline
as lesson 05-03 and for the same reason: a test nobody has seen fail is a test
nobody has any evidence about. Keep the broken version around long enough to see
red, then fix it and see green.

**Seed the source, and print the seed.**

```cpp
rc::test::Source source(seed);
// ... and report source.seed() with every failure
```

Then a failure is a command somebody can run. Use a fixed seed in the suite so
the same values run every time, and a rotating seed in a longer nightly job that
records which one it used.

**Shrink before reporting.**

```cpp
const T smallest = rc::test::shrink(failing, simpler, holds);
```

Propose candidates that are shorter first and simpler second, take the first one
that still fails, and repeat until nothing proposed fails. The one thing a
shrinker must never do is return something that passes: every value it hands
back has to have failed on the way.

A shrunk counterexample is usually enough to see the bug without a debugger,
which is the difference between a tool people use and one they turn off.
