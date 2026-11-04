id: E-DEBUG-0008
title: A check macro that evaluates its argument twice
match: RC_EXPECT is still there, and evaluates its condition exactly once
platforms: linux, windows
teaches: 04-06-the-check-that-was-not-in-the-release-build
---

## Symptom

A counter advances twice per loop. A queue is read twice and one reading is
lost. A function with a side effect in it is called once in the release build
and twice in the debug one, or twice when the check fails and once when it
holds.

The call site reads as one call.

## Cause

A macro that names its argument more than once evaluates it more than once.

```cpp
#define BAD_EXPECT(cond, msg) \
  ((cond) ? true : (report(#cond, msg), (cond)))     // twice on failure
```

Anything written at the call site runs again: `pop()`, `++index`,
`sensor.read()`, a function that opens a connection. And it runs only on the
failing path, so the double call appears exactly when something is already
going wrong.

The same hazard produced the classic `MAX(a, b)` bug, and it is why a macro
that cannot avoid naming its argument twice should be a function instead.

Measured on `RC_EXPECT`, which names its condition once: a counting function
inside it ran **once** when the condition held and **once** when it did not.

A related failure in the same family is a macro that reports only that
something was wrong. `rc::core::CheckFailure` carries the expression as
written, plus the file and line, because a reader who is told only that a
check failed is no better off than the symptom left them.

## Fix

**Name the argument once.** The conditional operator does it naturally:

```cpp
#define RC_EXPECT(condition, message) \
  ((condition) ? true : ::rc::core::report_check_failure(...))
```

`#condition` stringises it, which is not an evaluation.

**Test it with a side effect.** A counter incremented inside the check, on both
the holding and the failing path, is two lines and is the only thing that
catches this.

**Wrap the macro around a function.** Keep the macro to capturing `__FILE__`,
`__LINE__` and `#condition`, which only a macro can do, and put every other
behaviour in a real function where the ordinary rules apply.

**Prefer a function entirely when you do not need the location.** A macro is
the wrong default and the right tool only for the three things above.
