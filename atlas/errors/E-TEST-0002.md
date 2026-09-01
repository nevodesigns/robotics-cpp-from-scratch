id: E-TEST-0002
title: A check that could never have failed
match: every broken implementation is caught
match: uncaught:
platforms: linux, windows
teaches: 05-03-the-test-that-catches-it
---

## Symptom

A green suite over code that is wrong. Coverage is high, the tests are numerous,
and a bug ships anyway, in a function the tests very clearly exercise.

Read one of those tests closely and it turns out to assert something that was
going to be true whatever the function did.

## Cause

The check does not depend on the thing it is testing. The usual forms:

```cpp
RC_CHECK(limiter.apply(0.0, 1.0, 0.1) >= 0.0);   // true for almost anything
RC_CHECK(result == result);                       // true for everything
RC_CHECK(!path.empty());                          // true once anything is added
```

None of these separate a correct implementation from a wrong one, so none of
them can ever fail, so none of them is a test. Running them costs time and
produces a number that makes people confident.

There is a subtler version worth knowing, because it looks like diligence.
Asserting against a value **copied out of the implementation** rather than
worked out independently is the same problem: it will follow the implementation
wherever it goes, including into a bug.

## Fix

For every check, ask: **what would have to be wrong for this to fail?**

If the answer is "nothing I can think of", delete it. If the answer is a
specific mistake, the check is worth having and the mistake is worth naming in
the test's name.

The mechanical version of that question is mutation: break the implementation on
purpose, one thing at a time, and see whether the suite notices. A check that
survives every mutation is not testing anything.

Lesson 05-03 does this the other way round, which is easier to arrange and
answers the same question. Six implementations, each wrong in one specific way,
and a set of checks that must reject all six:

| check | catches |
|---|---|
| arrives within one step | steps past the target for ever |
| limits a distant target | does not limit at all |
| moves downward too | only ever moves upward |
| stays once arrived | will not stay where it arrived |
| a step of zero moves nothing | moves when asked to move by nothing |
| exactly one step away | wrong only at that boundary |

Every column of that table needs a row, and a row that catches no column is a
check that could never have failed.

One more thing the same suite has to require, or the cure is worse than the
disease. See `E-TEST-0003`.
