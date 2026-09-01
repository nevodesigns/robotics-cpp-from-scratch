# Writing the Test That Catches It

> By now you have read several hundred tests and written none. Every lesson so
> far handed you a failing suite. This one hands you six broken implementations
> and asks you to write the suite that tells them apart.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 05-02, 01-02

## The Problem

A green suite over code that is wrong is not unusual. Coverage is high, the
tests are numerous, and the bug ships anyway, from a function the tests very
clearly exercise.

Read one of those tests closely and it usually turns out to assert something
that was going to be true whatever the function did:

```cpp
RC_CHECK(limiter.apply(0.0, 1.0, 0.1) >= 0.0);   // true for almost anything
RC_CHECK(!path.empty());                          // true once anything is added
```

Neither separates a correct implementation from a wrong one. Neither can ever
fail. Running them costs time and produces a number that makes people confident.

So the question this lesson is about is not "how do I write a test". It is:
**how do I know whether the test I wrote is worth anything?**

## The Concept

### The standard: what would have to be wrong for this to fail

Ask it of every check. If the answer is "nothing I can think of", delete the
check. If the answer is a specific mistake, keep it and put that mistake in the
test's name.

The mechanical version is **mutation**: break the implementation on purpose, one
thing at a time, and see whether the suite notices. A check that survives every
mutation is testing nothing.

This lesson does it the other way round, which is easier to arrange and answers
the same question. You are given the rate limiter from lesson 01-02 behind an
interface, one implementation that is correct, and six that are each wrong in
exactly one way. Your job is a set of checks that **rejects all six and accepts
the one**.

### Both directions, and the second is the one people forget

A suite that rejects correct code is worse than no suite.

The damage is not the wasted hour. It is that the next failure gets the same
treatment, because everybody has learned that a red result means the test is
wrong. That is how a suite stops being believed, and after that it is only
costing time.

The most common cause is comparing fractional numbers for exact equality, which
lesson 00-04 explained and which will reject a correct implementation for its
last few bits. Compare with a tolerance.

There is a lazy answer to this lesson that is worth understanding rather than
just avoiding. A `checks_pass` that simply returns `false` rejects all six
broken implementations and passes six of the suite's eight tests. It is
worthless, and requiring it to accept the correct one is precisely what closes
that door.

### The four questions that cover it

The six faults are not arbitrary. Each is caught by a different kind of
question, and between them they are most of what goes wrong in code that moves a
number toward another number:

- **Does it arrive?** Within one step of the target it should land on it, not
  step past and come back. That is the oscillation of `E-NUM-0005`.
- **Does it limit?** A distant target should be approached by one step, not
  reached.
- **Does it work in both directions?** Downward is a direction. A limiter that
  only ever adds passes every check written with the target above the current
  value, and the first example anybody writes moves upward.
- **What happens at the boundaries?** Already there. A step of zero. Exactly one
  step away.

### The boundary is the one that sampling misses

One of the six is wrong **only** when the remaining distance equals the step
size exactly.

Here is which check catches which fault, measured:

| check | catches |
|---|---|
| arrives from below | steps past for ever |
| arrives from above | steps past for ever |
| limits a far target | does not limit |
| moves downward too | does not limit, only moves upward |
| stays once arrived | will not stay, steps past for ever |
| a step of zero moves nothing | does not limit, ignores a zero step |
| **exactly one step up** | **wrong at that boundary** |
| **exactly one step down** | **wrong at that boundary** |
| never oversteps, over 101 positions | does not limit |
| never retreats, over 101 positions | steps past for ever, only moves upward |

The two boundary checks are the **only** things in the table that catch the
sixth fault. A sweep of a hundred and one evenly spaced starting positions does
not, and neither does any other property, because a boundary is a single point
in a continuous range and sampling lands on it with probability near zero.

Where do boundaries come from? Every comparison in an implementation decides
something:

```cpp
if (std::fabs(remaining) <= max_step) return target;
```

That line produces three cases worth testing, not one: less than, exactly, and
more than. Two of them are obvious. The third is the one that ships.

### Redundancy is not the same as uselessness

Look at the first two rows of the table: they catch the same single fault. If
you delete either, nothing slips through.

That does not make them worthless. It means they are redundant **against these
six faults**, and a seventh implementation that broke one direction and not the
other would separate them. The check worth questioning is one that catches
nothing at all, not one that shares a catch.

That distinction matters when using mutation testing on real code. "Removing
this check changes no result" is information about your mutants, not a verdict on
your test.

## Build It

Implement `checks_pass` in `exercise/solution.hpp`. Return true when the limiter
is right, false when it is not.

```
rcpp verify 05-03
```

The suite points it at all seven implementations and names each fault it fails
to catch, so a red result tells you which question you have not asked yet.

## Use It

The habit generalises past this exercise, and it is the one worth taking:

Before trusting a suite, break the code on purpose and see whether it notices.
It takes a minute, and it is the only way to find out whether the green was
telling you anything.

When you add a test to fix a reported bug, check that it **fails** on the old
code before you keep the fix. A regression test that never failed is a comment.

## What Breaks First

- **A check that could never have failed.** It costs time and produces
  confidence. See `E-TEST-0002`.
- **A check too strict to pass.** It rejects correct code and teaches everybody
  to disbelieve the suite. See `E-TEST-0003`.
- **The boundary that sampling never lands on.** See `E-TEST-0004`.

## Ship It

Nothing here graduates into a library. What you keep is a question to ask of
every check you write for the rest of the curriculum, and every suite in it was
written to survive that question.
