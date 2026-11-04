id: E-DEBUG-0006
title: A check that was compiled out of the build you shipped
match: an assert with NDEBUG defined does not evaluate its condition
platforms: linux, windows
teaches: 04-06-the-check-that-was-not-in-the-release-build
---

## Symptom

Something that cannot happen happens, on the robot, and never on your desk.
A speed limit is exceeded. An index goes out of range. A pointer that is
checked for null is dereferenced anyway.

The check is right there in the source. You can read it. It is correct.

## Cause

`NDEBUG` is defined by every release configuration, and `<cassert>` then
defines `assert` to nothing at all.

Not to a cheaper check. **To nothing.** The expression inside is not
evaluated, so anything it was doing stops happening.

Measured in a single translation unit, by including `<cassert>` twice with
different `NDEBUG` states, which is the one standard header you may do that
to:

| | counter after `assert(++n > 0)` |
|---|---|
| `NDEBUG` undefined | 1 |
| `NDEBUG` defined | **1**, the call never happened |

So a check with a side effect in it silently loses the side effect, and a
check with no side effect silently stops checking.

This is not a defect in `assert`. It is what `assert` is for: things you
believe cannot happen, verified while developing, free afterwards. The defect
is using it for things you believe **should not** happen, which must still be
noticed on the machine.

## Fix

**Decide which kind of check it is.** Cannot happen: `assert`, and accept that
it is gone in release. Should not happen: something that survives.

**Use a check that is not conditional on anything.** `RC_EXPECT` from
`rc/core/expect.hpp` has no `NDEBUG` in it, evaluates its condition exactly
once, and reports through an installable handler.

**Never put a side effect inside an assert.** `assert(advance() == 0)` works
in debug and stops advancing in release. If the call matters, call it, then
assert on the result.

**Do not define NDEBUG yourself to "turn checks off".** It turns off every
assert in every header you include, including the standard library's.

**Check what your release build actually defines.** `-DNDEBUG` is added by
CMake's Release and RelWithDebInfo configurations, and by almost every other
build system, without being mentioned anywhere in your own files.
