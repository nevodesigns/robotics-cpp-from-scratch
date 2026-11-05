id: E-TSAN-0003
title: ThreadSanitizer says unlock of an unlocked mutex, after a try_lock_for that worked
match: (unlock of an unlocked mutex|pthread_mutex_clocklock)
platforms: linux
teaches: 07-09-two-locks-neither-moves
---

## Symptom

The thread sanitizer reports, intermittently, in a correct program:

```
WARNING: ThreadSanitizer: unlock of an unlocked mutex (or by a wrong thread)
    #2 std::timed_mutex::unlock() ...
```

on a `std::timed_mutex` that one thread locked and another acquired with `try_lock_for`.
It appears in some runs and not others, because it only fires when the timed acquisition
**succeeds**, and whether it does depends on timing.

## Cause

A tool limitation, the sibling of `E-TSAN-0002`. `timed_mutex::try_lock_for` is
`try_lock_until` on a `steady_clock` deadline, and in libstdc++ from GCC 10 that calls
`pthread_mutex_clocklock`. Neither gcc 11's nor clang 14's thread sanitizer intercepts
it, so a lock taken that way is invisible to them, and the matching `unlock` looks like an
unlock of a mutex nobody holds.

Measured on a twelve line program, a holder thread and a thread that tries for the lock
with a 500 ms limit, 15 runs each:

| call | gcc 11 | clang 14 |
|---|---|---|
| `try_lock_for` (a `steady_clock` limit) | reports in **15 of 15** runs | reports in **15 of 15** runs |
| `try_lock_until` with a `system_clock` deadline | 0 of 15 | 0 of 15 |

Clang 14 intercepts `pthread_cond_clockwait`, which `E-TSAN-0002` is about, and not this
one, so a program can be clean in one place and not the other.

## Fix

Do not change correct code to satisfy a tool, and do not switch production timeouts to
`system_clock`, which jumps when the clock is adjusted.

In a test, or anywhere a lock is wanted with a limit, poll a plain `try_lock` against a
`steady_clock` deadline. Both sanitizers see `try_lock`:

```cpp
bool lock_within(std::mutex& mutex, std::chrono::milliseconds limit) {
  const auto stop = std::chrono::steady_clock::now() + limit;
  while (!mutex.try_lock()) {
    if (std::chrono::steady_clock::now() > stop) return false;
    std::this_thread::yield();
  }
  return true;
}
```

Measured: lesson 07-09's suite under clang 14's thread sanitizer reported in **7 of 25**
runs with `try_lock_for`, and in **0 of 30** with this, and 0 of 30 under gcc 11's.

An intermittent report that lines up with a timed lock succeeding is the signature. Check
it with a twelve line program before spending a day on it.
