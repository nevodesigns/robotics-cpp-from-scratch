id: E-THREAD-0001
title: A torn read, a value assembled from two different writes
match: WARNING: ThreadSanitizer: data race
match: expected .*torn.* to equal
platforms: linux, windows
teaches: 07-01-threads-and-the-torn-read
---

## Symptom

A consumer occasionally sees a value that was never written: a pose whose x came
from one reading and whose y came from the next, a robot at a position it was
never in. It happens rarely, never while anybody is watching, and never in
testing.

## Cause

A structure larger than one machine word is written in several stores. Writing a
pose is three separate stores, and a reader with no synchronisation can arrive
between any two of them.

This is a data race, which the standard calls undefined behaviour rather than a
wrong answer. The distinction matters: a compiler entitled to assume no races may
keep a value in a register across a loop, so a thread can fail to see another
thread's write at all, permanently.

## Fix

Guard every access, reads included, with the same mutex:

```cpp
void publish(const Reading& reading) {
  const std::lock_guard<std::mutex> held(mutex_);
  latest_ = reading;
}

Reading latest() const {
  const std::lock_guard<std::mutex> held(mutex_);
  return latest_;
}
```

One unguarded reader is enough to bring the tearing back, so the rule is every
access or none.

volatile does not help. It stops the compiler eliding accesses, which matters for
a hardware register, and says nothing about atomicity or ordering between
threads. A volatile pose still tears.

On the path that must finish by a deadline, a mutex is the wrong tool for a
different reason: waiting on it is unbounded. That is what lock free structures
are for.
