id: E-THREAD-0015
title: A task submitted to a pool that had shut down, and was never run
match: (task was never run|submitted after shutdown|outstanding for ever|wait_idle never returns)
platforms: linux, windows
teaches: 07-10-how-many-workers
---

## Symptom

`wait_idle` never returns, or returns only on its timeout, and one task never ran. It happens
at shutdown, rarely, and a rerun is fine.

## Cause

A race between submitting and shutting down. The pool counts a task, then pushes it. If
shutdown closes the queue in between, the workers drain what is there and leave, and the task is
pushed into a queue nobody reads, counted as outstanding for ever.

```cpp
{ lock; if (!accepting_) return false; ++outstanding_; }   // lock released
queue_.push(task);                                         // shutdown can close the queue before this
```

This is a real flaw, found in this lesson's own first draft by reading `submit` against
`shutdown` and not by running it: the race window is a few instructions and the suite passed.

## Fix

Make the decision to accept and the push one step, under the same lock that shutdown takes to stop
accepting:

```cpp
bool submit(Task task) {
  const std::lock_guard<std::mutex> held(mutex_);
  if (!accepting_) return false;
  ++outstanding_;
  queue_.push(task);
  return true;
}
```

Then `shutdown` either runs before the lock, in which case the submit is refused and nothing is
queued, or after it, in which case the task is already in the queue and is drained before the
workers leave. There is no third case.

The lock order is pool then queue, in `submit` only. Nothing takes them the other way, which is
what lesson 07-09's levels would check.

Count the task before queueing it for the same reason: a worker can finish it the instant it is
pushed, and a count that had not gone up yet would go below zero.
