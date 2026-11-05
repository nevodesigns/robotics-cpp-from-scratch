id: E-THREAD-0018
title: A mutex that got slower as readers were added
match: (slower as readers|reads per ms fell|readers contend on one lock|one lock for every read)
platforms: linux, windows
teaches: 07-12-many-readers-one-writer
---

## Symptom

A value read by many threads and written by one is behind a mutex, and the system gets *slower*
when more threads read it. Adding a core makes the total read rate fall. A profile shows the
time in `lock` and `unlock`, not in any work.

## Cause

Every read takes the same lock, so readers are serialised, and the lock's cache line moves
between cores on every acquisition.

Measured on gcc 11, a 32 byte pose written every 20 microseconds, 300 ms, reads per millisecond
across all readers:

| readers | 1 | 4 | 8 |
|---|---|---|---|
| mutex | 40774 | 10093 | 8503 |
| shared_mutex | 37521 | 5786 | 6351 |
| seqlock | 204873 | 790348 | 914603 |

Eight readers on the mutex got **a fifth** of what one did. The total fell as threads were
added. The seqlock scaled the other way, because a reader takes no lock and writes nothing, so
there is nothing for readers to contend for.

Lesson 07-01 is right that a mutex is correct for a value read occasionally. This is the
other case, one read very often by many.

## Fix

For a small plain value, one writer and many readers, use a seqlock:

```cpp
#include <rc/rt/seqlock.hpp>

rc::rt::Seqlock<Pose> pose;
pose.write(next);              // the one writer
const Pose now = pose.read();  // any number of readers, no lock
```

Keep the mutex where the value is large, or has several writers, or is read rarely: there it
is simpler and costs nothing. Measure the readers you will actually have.
