id: E-THREAD-0011
title: A robot that stopped, with no crash, no message, and no CPU use
match: (deadlock|both threads are waiting|every thread is blocked on a lock|stopped with no cpu use)
platforms: linux, windows
teaches: 07-09-two-locks-neither-moves
---

## Symptom

The process is alive and does nothing. It does not crash, it prints nothing, and it
uses no CPU. A debugger attached to it shows every thread blocked in a lock call, two of
them each waiting for a mutex the other holds.

```
Thread 1:  waiting for mutex B  (holds A)
Thread 2:  waiting for mutex A  (holds B)
```

## Cause

A deadlock needs a cycle: one thread takes A and then B, another takes B and then A. If
the first holds A and the second holds B, each waits for the other for ever.

Measured, gcc 11, real threads, with the interleaving forced and a limit on the wait so
the test could end: with opposite orders, **at least one thread gave up every time**, and
in some runs both did. With the same order in both threads, none did.

Lesson 07-01 says to take several locks with `std::scoped_lock`. Measured: two threads
doing `scoped_lock(a, b)` and `scoped_lock(b, a)`, 500 acquisitions each, both finished.

## Fix

Give every mutex a fixed position in an order, write the order down, and take locks only
in increasing position.

```cpp
const std::scoped_lock both(a, b);        // takes both, in an order it chooses
```

or, when the locks are not taken together, give each a level and check it, so a wrong
order is a report and not a hang:

```cpp
#include <rc/rt/level_mutex.hpp>

rc::rt::LevelMutex drive(1);
rc::rt::LevelMutex arm(2);
const rc::rt::OrderedLock first(drive);
const rc::rt::OrderedLock second(arm);
if (!second.ok()) { /* the order was wrong; nothing was taken */ }
```

Hold a lock for as short a time as you can, and call nothing you do not own while you
hold one.
