# The Check That Was Not in the Release Build: assert, NDEBUG, and What to Use Instead

> A counter incremented inside `assert(...)` read 1 with NDEBUG undefined, and
> still read 1 after a second call with NDEBUG defined. The call never
> happened.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-09, 04-04

## The Problem

`assert` is the first checking tool anybody meets, and the thing nobody
mentions is that it is **compiled out of the build you ship**.

Something that cannot happen happens, on the robot, and never on your desk. A
speed limit is exceeded. The check is right there in the source. You can read
it. It is correct. It is also not in the binary.

## The Concept

### What NDEBUG actually does

`NDEBUG` is defined by every release configuration, including this
curriculum's, and `<cassert>` then defines `assert` to nothing at all.

Not to a cheaper check. **To nothing.**

That is demonstrable in a single file, because `<cassert>` is the one standard
header you may include more than once to get different behaviour, and this is
exactly what it is for:

| | counter after `assert(++n > 0)` |
|---|---|
| `NDEBUG` undefined | 1 |
| `NDEBUG` defined | **1**, the call never happened |
| undefined again | 2 |

So a check with a side effect in it silently loses the side effect, and a check
with no side effect silently stops checking. Both compilers agree.

Note that nothing in your own files says `NDEBUG` anywhere. CMake's Release and
RelWithDebInfo configurations add `-DNDEBUG`, and so does almost every other
build system, without mentioning it.

### Which is not a defect in assert

This is what `assert` is **for**: things you believe cannot happen, verified
while you develop, free afterwards. For a bounds check in a hot loop that is a
good trade, made deliberately.

The defect is using it for the other kind of check.

> `assert` is for things you believe **cannot** happen.
> Something else is needed for things you believe **should not** happen.

A precondition on a motor command is the second kind. It must still be noticed
on the machine.

### Three decisions, each the opposite of assert's

**It is never compiled out.** There is no `NDEBUG` anywhere in
`rc/core/expect.hpp`.

**It reports rather than aborts.** A moving machine does not become safer by
stopping its own control loop: it becomes a machine with momentum and no
controller. The default handler prints and returns, and the expression answers
false. `abort_on_failure` exists and is a deliberate choice for the cases where
carrying on is worse.

**It is an expression**, so the caller has to say what happens next:

```cpp
if (!RC_EXPECT(requested <= limit, "commanded speed above the configured limit"))
  return limit;
```

Longer than an assert, and it says who decided. Measured: a request of 9.0
against a limit of 2.0 returned 2.0 and reported once; 1.5 returned 1.5 and
reported nothing; the limit itself is not a violation and reported nothing.

Reporting and deciding are different jobs, and `assert` conflates them.

### Naming the condition exactly once

A macro that names its argument more than once evaluates it more than once, and
on the failing path only, so a `pop()` or a `sensor.read()` written at the call
site runs twice exactly when something is already going wrong. It is the
classic `MAX(a, b)` hazard.

The conditional operator avoids it naturally, and `#condition` stringises
rather than evaluates. Measured: a counting function inside `RC_EXPECT` ran
**once** when the condition held and **once** when it did not.

Keep a macro to the three things only a macro can do, which are `__FILE__`,
`__LINE__` and `#condition`, and put every other behaviour in a real function.

### Where the handler lives

```cpp
inline CheckHandler& check_handler_storage() {
  static CheckHandler handler = &log_failure;
  return handler;
}
```

A function local static inside an inline function, which lesson 04-04
established is **one object** across every translation unit. A plain global in
a header would be one per unit, so installing a handler in one file would leave
every other file reporting somewhere else.

And a null handler is refused and replaced with the silent one, because a null
one turns a reporting mechanism into a worse version of the thing it replaced:
the first violation becomes a crash rather than a report.

## Build It

Implement `set_check_handler`, `report_check_failure` and `safe_speed` in
`exercise/solution.hpp`.

```
rcpp verify 04-06
```

The suite toggles NDEBUG inside one file and watches a side effect disappear,
counts how many times `RC_EXPECT` evaluates its condition on both paths, checks
that a failure reports the expression, file and line, clamps a commanded speed
both ways, and installs a null handler to see it refused.

## Use It

**Decide which kind of check it is**, every time. Cannot happen: `assert`.
Should not happen: `RC_EXPECT`.

**Never put a side effect inside an assert.**

**Install a handler that does something in production.** The default prints to
stderr, which on a robot is frequently nowhere. Point it at your log, your
telemetry, or a counter you can read over the network.

**Keep the handler out of the decision.** The handler records; the return value
decides. A handler that throws or aborts takes the choice away from every call
site at once.

**Route a safety decision to whatever owns safety**, which from phase 14 is the
rate limiter and the watchdog rather than the function that noticed.

**Check what your release configuration defines.** It is adding `-DNDEBUG`
whether or not you knew.

## What Breaks First

- **A check compiled out of the build you shipped.** See `E-DEBUG-0006`.
- **A precondition that aborted the control loop.** See `E-DEBUG-0007`.
- **A macro that evaluates its argument twice.** See `E-DEBUG-0008`.

## Ship It

`CheckFailure`, the handlers, `RC_EXPECT` and `safe_speed` join `rc::core`.
From here on, anything in this curriculum that must still be checked on the
robot is written with `RC_EXPECT`, and `assert` is kept for the things that
genuinely cannot happen.
