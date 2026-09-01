id: E-TEST-0003
title: A check too strict to pass, which rejects correct code
match: the checks accept an implementation that is right
platforms: linux, windows
teaches: 05-03-the-test-that-catches-it
---

## Symptom

A test fails and the code is right. Somebody investigates, finds nothing, and
adjusts the test until it passes.

That is the real damage, and it is not the wasted hour. It is that the next
failure gets the same treatment, because the suite has taught everybody that a
red result means the test is wrong.

## Cause

Most often, comparing fractional numbers for exact equality:

```cpp
RC_CHECK(limiter.apply(0.95, 1.0, 0.1) == 1.0);   // fails on arithmetic
```

Two calculations that should agree will differ in their last bits. Lesson 00-04
is about why, and `E-NUM-0003` is the entry for it.

The other forms are all versions of asserting more than the specification says:

- **Over specifying a value.** The requirement is "no more than one step"; the
  test demands exactly one step, and a legitimate implementation that takes a
  smaller step near the target is rejected.
- **Depending on order** where the specification does not promise one.
- **Depending on timing.** A test that requires something to finish inside ten
  milliseconds fails on a loaded machine, and lesson 07-03 measured how wide
  that distribution really is.
- **Pinning an error message** rather than the failure it reports.

## Fix

Assert what the specification promises and nothing more.

```cpp
RC_CHECK(std::fabs(limiter.apply(0.95, 1.0, 0.1) - 1.0) < 1e-9);
```

A suite has to be judged from both sides at once, and this is the half that gets
forgotten:

- It must **reject** every implementation that is wrong. See `E-TEST-0002`.
- It must **accept** every implementation that is right.

The second is why a test that always reports failure is not a safe default. In
lesson 05-03 a `checks_pass` that simply returns false rejects all six broken
implementations and passes six of the suite's eight tests, and it is worthless,
which is exactly why the suite also requires it to accept the correct one.

When a test fails and you believe the code, resist changing the test until you
can say which of the two is wrong. If it is the test, write down in its name or
a comment what it was over specifying, or it will be rewritten back.
