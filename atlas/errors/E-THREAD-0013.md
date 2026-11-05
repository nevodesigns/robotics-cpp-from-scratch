id: E-THREAD-0013
title: A thread that deadlocked with itself
match: (Resource deadlock avoided|resource_deadlock_would_occur|locked a mutex it already held)
platforms: linux, windows
teaches: 07-09-two-locks-neither-moves
---

## Symptom

One thread, one lock, and the program hangs. Or, with an error checking mutex or a join
on itself, an exception:

```
terminate called after throwing an instance of 'std::system_error'
  what():  Resource deadlock avoided
```

## Cause

A thread locks a `std::mutex` it already holds. That is undefined behaviour, and in
practice a hang: the thread waits for itself.

It is easy to write by accident. A function takes a lock and calls a helper that takes
the same lock:

```cpp
void set_target(double v) { const std::lock_guard<std::mutex> g(m_); update(v); }
void update(double v)     { const std::lock_guard<std::mutex> g(m_); target_ = v; }   // again
```

Neither function is wrong alone. This is the same rule as a lock order: the same mutex
twice on one thread is a cycle of length one.

Measured: a lock at a level already held, **including the same mutex**, was refused and
counted, where a plain `std::mutex` would have hung.

## Fix

Take the lock once, at the outer function, and have the inner one assume it is held. Name
it so nobody calls it without:

```cpp
void set_target(double v) { const std::lock_guard<std::mutex> g(m_); update_locked(v); }
void update_locked(double v) { target_ = v; }       // caller holds m_
```

A `std::recursive_mutex` allows the second lock and hides the design question, which is
which function owns the invariant. Reach for it last.

The level check from lesson 07-09 reports this, and so does `E-THREAD-0011`'s fix, as an
order violation at the first attempt.
