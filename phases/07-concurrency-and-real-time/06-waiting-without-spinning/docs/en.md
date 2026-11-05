# Waiting Without Spinning: Condition Variables, the Lost Wakeup, and Stopping a Thread That Is Asleep

> A consumer waiting 100 ms for one item used 100.05 ms of CPU by asking again, and
> 0.04 ms by sleeping until it was told.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 07-01, 07-02, 03-07

## The Problem

Lesson 07-02 built a queue whose `pop` returns false when it is empty, and left the
next question unasked: what does the consumer do then?

The answer most people write first is a loop. Ask again, and again, until something
is there. It works, and what it costs is the subject of the first section.

Behind it is a harder problem. A thread that has nothing to do has to be able to
**sleep**, and to be woken when there is something, and to be woken when the program
is shutting down. Get any of those three wrong and you have a robot that burns a
core doing nothing, or hangs on a message that was sent too early, or will not exit.

## The Concept

### What asking again costs

Experiment, method stated: a consumer waits 100 ms for a producer to push one item,
three ways, with the consumer thread's own CPU time read from the operating system.
One 12 core machine, gcc 11, run once and outside the suite, because CPU time is a
property of the machine that day.

```
                            wakeups          CPU the consumer used, of 100 ms
   spin, with yield         231000 to 309000          100.05 ms
   poll every 1 ms                92                    1.85 ms
   condition variable              1                    0.04 ms
```

Spinning used the whole core to find out that nothing had happened. `yield` gives up
the time slice and the thread is runnable again immediately, so it asks as fast as
the machine allows. Polling every millisecond is cheap, and still wakes once a
millisecond for as long as nothing happens, and an item that arrives just after a
poll waits for the next one. The condition variable woke once, when there was
something to do.

The suite asserts the **counts**: the waiter went to sleep once, or at most three
times to allow for a spurious wakeup, and the poller asked more than a hundred times.

`E-THREAD-0006`.

### A condition variable sleeps until told

```cpp
std::unique_lock<std::mutex> lock(mutex_);
while (items_.empty() && !closed_) ready_.wait_until(lock, deadline);
```

`wait_until` does three things as one step: it releases the mutex, puts the thread
to sleep, and takes the mutex back when it wakes. That is why it needs a
`std::unique_lock` and not a `lock_guard`: a lock that can be released and retaken.

Everything that follows is one rule. **The thing you are waiting for is state, kept
under a mutex. The condition variable only says "look again".**

### A notification is not remembered

```cpp
signal.notify_one();                         // nobody is waiting yet
signal.wait_for(lock, milliseconds(50));     // waits for something that already happened
```

```
  notify first, then wait 50 ms:
    with no condition checked   timed out, the notification was lost
    checking the state first    returned at once
```

A notification wakes whoever is waiting at that instant and is gone. The wait that
followed it had nothing to look at, so it slept until the timeout. This is a lost
wakeup, and it appears rarely, at start-up, on someone else's machine.

The queue is built the second way. An item pushed before anyone asks is found by
looking: no notification is needed and nobody sleeps. The suite pushes first and
checks that the consumer never went to sleep.

`E-THREAD-0005`.

### A wakeup is not the condition

Why `while` and not `if`:

```
  two asleep, one item pushed, then close():
    took the item   1
    told it was closed   1
```

Two consumers, one item. One took it. The other must carry on waiting, and a wait
written as an `if` would have treated its wakeup as permission and read an empty
queue. Measured with `notify_all` in a separate experiment, the loser woke, found
nothing, and went back to sleep. The standard also allows a wait to return when
nobody notified at all, a **spurious wakeup**. Either way the answer is the same:
the wait is a loop that checks the state again.

### Stopping a thread that is asleep

```
  three consumers asleep, then close():
    all three returned within 1 ms, where the wait was 5000 ms
```

A thread blocked on a condition variable cannot be told to stop by anything except
that condition variable. A flag does not reach it:

```cpp
running = false;      // the consumer is asleep and will never look
worker.join();        // so this waits for ever
```

`close()` sets the state and wakes **everyone**. `notify_one` would wake one and leave
the others asleep, and a program cannot exit while a thread it started is still
waiting. A closed queue still hands out what it holds before it reports closed, so a
shutdown does not drop work that was already accepted.

`E-THREAD-0007`.

### Every wait has a deadline

```
  pop_for on an empty queue with a 30 ms deadline: timeout after 30 ms, 1 sleep(s)
```

`pop_for` is the only way to take an item, and a timeout is a result like any other,
beside `kGot` and `kClosed`. A control path that waits for ever on a producer that has
died has stopped controlling. A test that waits for ever on a queue nobody wrote to is
a hung test. And a consumer that wakes at least occasionally can notice a stop request
even if a notification is somehow missed.

## Build It

Three methods of `BlockingQueue`.

`push` changes the state under the lock, in a block of its own, then notifies after
the lock is released, so the thread that wakes is not immediately blocked on a mutex
the pusher still holds.

`pop_for` works out its deadline once, takes a `std::unique_lock`, loops while the
queue is empty and not closed, counting each sleep in `waits_`, and gives up when the
wait times out. After the loop, **whichever way it ended**, an item that is there is
handed out: one that arrived at the instant of the timeout still counts.

`close` sets the flag under the lock and calls `notify_all`.

The stubs drop the item, time out at once, and wake nobody. The suite gives every
thread a deadline, so a half-finished queue fails quickly and never hangs.

## Use It

```cpp
#include <rc/rt/blocking_queue.hpp>

rc::rt::BlockingQueue<Reading> queue;
Reading reading;
using Queue = rc::rt::BlockingQueue<Reading>;
if (queue.pop_for(reading, std::chrono::milliseconds(100)) == Queue::Result::kGot) {
  // use it
}
```

Lesson 07-02's queue is still the right thing where a producer must never block. The
two answer different questions: that one never makes the producer wait, this one lets
the consumer sleep.

Run threaded code under the thread sanitizer, as lesson 07-01 says, and expect this:

```
WARNING: ThreadSanitizer: double lock of a mutex
    #4 BlockingQueue<int>::push(int const&) ...
```

on **GCC 11**, in a program that is correct. Experiment, method stated: twelve lines,
a waiter and a producer, built with each compiler's sanitizer.

| wait used | gcc 11 | clang 14 |
| --- | --- | --- |
| `wait_until(steady_clock)` | **2 warnings** | 0 |
| `wait()` with no timeout | 0 | 0 |
| `wait_until(system_clock)` | 0 | 0 |

`wait_until` with a `steady_clock` deadline calls `pthread_cond_clockwait`, which
GCC 11's sanitizer does not intercept, so it never sees the mutex released during the
wait. This lesson's own suite reported 11 warnings under it, and none under clang 14's
on three runs. Do not change the program to satisfy the tool: `system_clock` would
silence it and make the timeout jump when the clock is adjusted. Use clang for the
thread sanitizer when code waits with a timeout.

`E-TSAN-0002`.

Two limits.

**This is a queue with a mutex in it.** The producer can wait for the consumer's lock,
for a moment. It is the wrong thing on a path with a millisecond to spend, and
lesson 07-02 is the right one there.

**It has one lock for everything.** Many producers and many consumers on one queue
contend for it, which is lesson 07-05's kind of cost, and a bounded version that makes
a producer wait when the queue is full is the natural next step.

## What Breaks First

`E-THREAD-0005` is the wakeup that never came, because the notification was sent before
the wait and the state was never checked.

`E-THREAD-0006` is the thread that waited by asking again and held a whole core to do
it.

`E-THREAD-0007` is the program that would not exit, because a thread was asleep and
nobody woke it.

## Ship It

`rc/rt/blocking_queue.hpp`, in `namespace rc::rt`.

From here on, a thread in this curriculum that waits for another sleeps on a condition
with a deadline and is woken by `close`, and nothing spins.
