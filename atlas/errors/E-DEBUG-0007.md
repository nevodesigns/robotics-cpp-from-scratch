id: E-DEBUG-0007
title: A precondition that aborted the control loop
match: the caller decides what a violation means
platforms: linux, windows
teaches: 04-06-the-check-that-was-not-in-the-release-build
---

## Symptom

A robot stops dead in the middle of a movement, with an arm half extended or a
load part lifted, because a check somewhere decided the situation was
impossible. The log says which check. It does not say what the machine should
do now, because nothing decided that.

Or the opposite, after somebody removed the abort: the violation is noticed,
printed, and nothing acts on it, so the out of range command goes to the motor
anyway.

## Cause

Reporting and deciding are different jobs, and `assert` conflates them: it
notices and it terminates, and the terminating is not configurable.

Terminating is occasionally right and usually not. A moving machine does not
become safer by stopping its own control loop: it becomes a machine with
momentum and no controller. What to do about a violated precondition depends
on the machine, the state, and the violation, and the only code that knows all
three is the caller.

So the check answers, and the caller decides:

```cpp
if (!RC_EXPECT(requested <= limit, "commanded speed above the configured limit"))
  return limit;
```

Measured, that shape clamps and reports and keeps running: a request of 9.0
against a limit of 2.0 returned 2.0 and produced one report, and a request of
1.5 returned 1.5 and produced none. A request exactly at the limit is not a
violation and produced none either.

## Fix

**Make the check an expression, not a statement.** Then the call site cannot
avoid saying what happens next, and a reader can see the decision.

**Default to reporting, not aborting.** `rc::core::expect.hpp` installs
`log_failure` by default. `abort_on_failure` exists and is a deliberate
choice, for the cases where carrying on is worse than stopping.

**Route the decision to whatever owns the machine's safety.** Clamping in the
function is fine for a speed. A violated geometric precondition on an arm may
need the motion planner, not the joint controller. Lesson 14-02's rate limiter
and watchdog are where that lives.

**Install a handler that does something in production.** The default prints to
stderr, which on a robot is frequently nowhere. Point it at your log, your
telemetry, or a counter you can read over the network.

**Keep the handler out of the decision.** The handler records; the return value
decides. A handler that throws or aborts takes the choice away from every call
site at once.
