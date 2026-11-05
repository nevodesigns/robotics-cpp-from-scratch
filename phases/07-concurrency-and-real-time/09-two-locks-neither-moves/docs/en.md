# Two Locks, Neither Moves: Deadlock, and Finding It Without Waiting for the Hang

> The same code, with opposite lock orders, deadlocked 3739 times in one run and
> not once in three runs at the next setting. A run that does not hang proves
> nothing.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 07-01, 07-07, 02-03

## The Problem

Lesson 07-01 said that locking by hand is how a robot deadlocks, and that
`std::scoped_lock` avoids it. It did not show one.

A deadlock is the worst kind of failure a robot has. It does not crash, so there is no
core file. It prints nothing. It uses no CPU. The process is alive, every thread is
waiting, and the arm stops where it is. And it shows up in the field and not on the
bench, for a reason this lesson measures.

## The Concept

### Two threads, two locks, opposite orders

```
thread 1:  lock A, then lock B
thread 2:  lock B, then lock A
```

If thread 1 holds A and thread 2 holds B at the same moment, each waits for the other
for ever. Neither can release what it holds, because releasing comes after the second
lock.

Experiment, method stated: each thread takes its first lock, waits until the other has,
and then asks for the second, with a 200 ms limit so the test can end. In a real deadlock
there is no limit.

```
  opposite order, interleaving forced: 1 of 2 threads gave up waiting
  (1 or 2: when the first gives up and lets go, the other gets through)
```

At least one thread gave up every time, and in some runs both did. With the same order in
both threads, none did. And two threads doing `scoped_lock(a, b)` and `scoped_lock(b, a)`,
500 acquisitions each, both finished: `scoped_lock` takes both at once in an order it
chooses, so the order you wrote does not matter.

`E-THREAD-0011`.

### A run that does not hang proves nothing

That was forced. Left alone, the same code, opposite orders, two threads in a loop, with
different amounts of unrelated work between passes. Three runs each, 4000 passes, and the
deadlocks seen:

| unrelated work between passes | run 1 | run 2 | run 3 |
| --- | --- | --- | --- |
| none | 129 | **3739** | 3387 |
| 10 us | 54 | 28 | 41 |
| 100 us | **0** | **0** | **0** |
| 1000 us | 0 | 3 | 16 |

The same bug, in the same code. Thousands in one run. None in three runs at 100 us. Back
again at a longer gap. A test suite that happened to run at the quiet setting would pass,
and the robot would hang on the first busy afternoon.

Lesson 07-01 measured the same of a torn read, and this is worse, because a torn read
corrupts a number and this stops the machine.

`E-THREAD-0012`.

### So do not wait for the hang

A deadlock needs a **cycle** of lock orders: one thread takes A then B, another takes B
then A. The cycle is visible in the order, on a single thread, whether or not two threads
ever overlapped.

So give every mutex a **level**, and refuse, and count, any lock taken at a level no
higher than one the thread already holds. Then A then B is always allowed, and B then A
is always caught, the first time it is attempted, with nothing deadlocked and no timing
involved.

```
  two threads, one after the other, never running together:
    drive then arm   violations 0
    arm then drive   violations 1
  Nothing deadlocked and nothing could have. The cycle was found in the order.
```

The two threads ran one after the other and could never have hung. The second was
refused anyway, because its order was the bug. This is what the Linux kernel's lock
dependency checker does, at a larger scale.

### The same mutex twice is the same rule

```cpp
void set_target(double v) { const std::lock_guard<std::mutex> g(m_); update(v); }
void update(double v)     { const std::lock_guard<std::mutex> g(m_); target_ = v; }
```

Neither function is wrong alone. Together, one thread locks a mutex it already holds,
which is undefined behaviour and in practice a hang: a thread deadlocking with itself. It
is a cycle of length one, so the level check refuses it by the same rule, and so it
refuses a second mutex at the same level, because an order that is not defined is not
safe.

`E-THREAD-0013`.

### The check changes how it ends

```
  the same opposite orders with levels: both threads finished (2 of 2), 1 order violation(s) reported
```

The same opposite orders, the same forced overlap, and nothing waits. The wrong order is
refused the moment it is tried, takes nothing, and cannot block. The thread that was
refused finishes, releases what it held, and the other proceeds.

### `thread_local`

The levels a thread holds are kept in a `thread_local` list:

```cpp
inline std::vector<int>& held_levels() {
  thread_local std::vector<int> held;
  return held;
}
```

`thread_local` gives each thread its own copy. No thread's list is touched by another, so
none needs a lock of its own, which matters in a class whose job is to be a lock. It is the sibling
of lesson 02-12's `static`: one copy per thread, where `static` is one per program.

## Build It

Two classes.

`LevelMutex::lock()` gets the thread's list, and if it is not empty and this level is at or
below the last one held, counts a violation and returns `false` **without locking**. Otherwise
it locks the real mutex and records the level. `unlock()` finds its level in the list and
erases it wherever it is, because unlocking in a different order from locking is allowed,
and then unlocks.

`OrderedLock` is lesson 02-03's guard with one addition: `ok()` says whether the lock was
taken, because here the answer can be no. A guard that was refused unlocks nothing, which
matters, because unlocking it would release somebody else's lock.

The stubs refuse everything. They cannot deadlock, and the suite names them by what they
refused.

## Use It

```cpp
#include <rc/rt/level_mutex.hpp>

rc::rt::LevelMutex drive(1);
rc::rt::LevelMutex arm(2);

const rc::rt::OrderedLock first(drive);
const rc::rt::OrderedLock second(arm);
if (!second.ok()) { /* the order was wrong; nothing was taken */ }
```

Pick the order once, write it next to the declarations, and let the check enforce it.

Run threaded code under the thread sanitizer, as lesson 07-01 says. Two things from
measurement:

**Prefer a polled `try_lock` to `timed_mutex::try_lock_for` in a test.** Under both gcc 11's
and clang 14's thread sanitizer, a `try_lock_for` that succeeds is reported as `unlock of an
unlocked mutex`: 15 of 15 runs on a twelve line program. `pthread_mutex_clocklock` is not
intercepted, so the tool cannot see the lock. This suite's first draft used it and clang's
reported in 7 of 25 runs; polling a plain `try_lock` against a `steady_clock` deadline
reported in 0 of 30 under each. `E-TSAN-0003`.

Do not move production timeouts to `system_clock` to quieten a tool, since that jumps when the
clock is adjusted.

Two limits.

**It finds the cycles it can see.** A lock order that goes through a callback, or a
condition variable it does not know about, is outside it. Hold locks briefly and call
nothing you do not own while you hold one.

**It checks order on a thread, not across processes.** Two programs sharing a file lock are
a different problem.

## What Breaks First

`E-THREAD-0011` is the robot that stopped with no crash, no message and no CPU use.

`E-THREAD-0012` is the deadlock that never happened in testing, with the table of how
often it did.

`E-THREAD-0013` is the thread that deadlocked with itself.

## Ship It

`rc/rt/level_mutex.hpp`, in `namespace rc::rt`.

From here on, a subsystem in this curriculum that holds more than one lock gives each a
level, writes the order down, and takes them in it.
