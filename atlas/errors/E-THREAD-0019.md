id: E-THREAD-0019
title: A shared_mutex that starved the writer
match: (starved the writer|writer starvation|writer never got in|shared_mutex made the writer wait)
platforms: linux, windows
teaches: 07-12-many-readers-one-writer
---

## Symptom

The reader-writer lock that was meant to fix a slow mutex made things worse. Readers are fast,
and the writer almost never gets in: the data goes stale, the sensor thread falls behind, and a
queue behind the writer grows.

## Cause

`std::shared_mutex` lets any number of readers hold the lock together, and a writer needs all of
them out. With enough readers arriving before the last one leaves, there is never a moment with
none, and the writer waits.

Measured, gcc 11, a writer that wanted to write about 14,000 times in 300 ms, by readers:

| readers | 1 | 4 | 8 |
|---|---|---|---|
| shared_mutex, writes done | 14121 | 8576 | **92** |
| mutex, writes done | 14377 | 13765 | 13019 |

With 8 readers the writer made **92 writes**, and **71** in a second run. The plain mutex gave it
fair turns. `shared_mutex` was also slower for the readers than the mutex it replaced: 5786 against
10093 reads per millisecond at 4 readers, because taking a shared lock still writes to shared
memory.

Whether a writer is preferred is up to the implementation. The standard does not say, so do not
write code that depends on either.

## Fix

Do not reach for `shared_mutex` because the access pattern is "many readers". Measure it, with the
readers you will have, for both the readers and the writer.

For a small value, a seqlock gives readers no lock at all and the writer never waits:

```cpp
rc::rt::Seqlock<Pose> pose;
```

Measured, 8 readers: the writer made 14585 writes and the readers 914603 reads per millisecond.

Count what the writer completes as well as what the readers do. A reader benchmark alone would
have called `shared_mutex` fine.
