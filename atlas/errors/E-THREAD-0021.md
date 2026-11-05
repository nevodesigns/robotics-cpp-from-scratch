id: E-THREAD-0021
title: get() threw, and it was the task's exception or a broken promise
match: (Broken promise|broken_promise|std::future_error|get\(\) rethrew)
platforms: linux, windows
teaches: 07-13-getting-an-answer-back
---

## Symptom

A call to `future.get()` throws, from a line that does nothing risky. Either the task's own exception
turns up in the caller, or a `std::future_error` with the message `Broken promise`.

## Cause

Two different things, and both are the design.

**The task's exception.** `get()` rethrows what the task threw. That is how a failure in another thread
becomes a failure the caller has to handle. Measured: a task that threw `runtime_error("sensor lost")`
made `get()` throw the same, and the pool reported `completed 2`, **`failed 0`**. The future caught the
exception and kept it for the caller, so the pool was never told. The two ways of reporting a failure
are separate: a plain task is counted by the pool (lesson 07-10), a task behind a future is delivered
to whoever holds the future.

**A broken promise.** If the promise or packaged task is destroyed before it was ever run, the future
is made ready with `future_error` and the code `broken_promise`. Measured: a task submitted to a pool
that had shut down was destroyed unrun, and `get()` threw `std::future_error: Broken promise` instead
of waiting for ever. Dropping a task without running it, a full queue or a closed pool, is reported to
the person waiting.

## Fix

Expect `get()` to throw and decide what it means:

```cpp
try {
  const int answer = result.get();
} catch (const std::future_error& e) {
  if (e.code() == std::future_errc::broken_promise) { /* the work was never run */ }
} catch (const std::exception& e) {
  /* the task itself failed */
}
```

And never call `get()` without a limit you chose. `get()` waits for ever on a task that never finishes,
so ask first:

```cpp
if (rc::rt::ready_within(result, std::chrono::milliseconds(100))) use(result.get());
```

`wait_for` never blocks longer than its argument. Measured: `ready_within` gave up at 40 ms on a task
held on a gate, and answered yes once it was released.
