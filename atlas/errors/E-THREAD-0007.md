id: E-THREAD-0007
title: The program that will not exit, because a thread is asleep and nobody woke it
match: (will not exit|thread is still waiting|hangs on join|never returns from join)
platforms: linux, windows
teaches: 07-06-waiting-without-spinning
---

## Symptom

Shutdown hangs. The main thread has asked everything to stop, joined most of the
threads, and is now stuck in `join` on one that never returns. Ctrl-C works and
nothing in the log says why. A test suite that starts consumers hangs at the end
instead of failing.

## Cause

A thread blocked on a condition variable cannot be told to stop by anything except
that condition variable. A plain flag does not reach it:

```cpp
running = false;      // the consumer is asleep and will never look
worker.join();        // so this waits for ever
```

A queue with no way to say "finished" leaves its consumers asleep for ever, and a
program cannot exit while a thread it started is still running.

Two shapes of the same mistake. `close()` that wakes **one** waiter, with
`notify_one`, leaves the others asleep: every waiter has to notice, so it has to be
`notify_all`. And `close()` that discards what is still queued drops work that was
already accepted.

Measured: three consumers asleep with a 5000 ms wait, then `close()`. All three
returned **closed** within 1 ms.

## Fix

Make shutdown a state of the queue, and wake everyone:

```cpp
void close() {
  { const std::lock_guard<std::mutex> held(mutex_); closed_ = true; }
  ready_.notify_all();
}
```

The wait loop checks `closed_` as well as emptiness, and a closed queue still hands
out what it holds before it reports closed, so a shutdown does not lose work.

Give every wait a deadline too. It is the second line of defence: a consumer that
wakes at least once a second can notice a stop request even if a notification is
somehow missed.
