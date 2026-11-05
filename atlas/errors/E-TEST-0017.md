id: E-TEST-0017
title: A race hook that made the test wait for itself
match: (waits for itself|hook took the lock|point inside the lock|killed by the timeout)
platforms: linux, windows
teaches: 07-11-the-race-the-tests-passed
---

## Symptom

A test using a race hook hangs until the test runner kills it. No failure message, no stack,
exit status 124 from `timeout`. It hangs on the first run, every time, which at least makes it
findable.

## Cause

The race point was placed while a lock was held, and the hook took the same lock.

```cpp
void submit() {
  const std::lock_guard<std::mutex> held(mutex_);
  race_point("submit.inside");                    // the hook runs here, with mutex_ held
}
const RaceHook hook("submit.inside", [&] { pool.shutdown(); });   // shutdown locks mutex_
```

The hook runs on the thread that reached the point, inside the call, with the lock held. It
calls `shutdown`, which locks the same mutex. A `std::mutex` locked twice by one thread waits
for itself.

Measured on a small program: a point outside the lock returned at once; a point inside the
lock with a hook that took it was killed by a 3 second timeout, exit 124.

The same rule applies inside the hook table. `race_point` copies the action out and runs it
after releasing the table's own lock, for the same reason: an action that reaches another race
point would otherwise wait for itself.

## Fix

Place race points where no lock is held, and have hooks call the public operations, which take
their own locks.

If the interleaving you want to test is genuinely inside a critical section, the point does not
belong there. The thing to test is another thread arriving while the lock is held, and that is
a different test: it waits on the lock, and `E-THREAD-0011`'s ordering applies.

A class built on lesson 07-09's `LevelMutex` would refuse this at once, as a lock at a level
already held, where a plain `std::mutex` hangs.
