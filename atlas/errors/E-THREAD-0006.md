id: E-THREAD-0006
title: A thread that waits by asking again, and holds a whole core to do it
match: (100 percent of a core|busy wait|busy-wait|polls wakes thousands)
platforms: linux, windows
teaches: 07-06-waiting-without-spinning
---

## Symptom

A robot with a quiet sensor pegs a core at 100 percent. The fans spin up, a laptop
battery drains, and a second thread that actually has work to do runs slower on a
machine with few cores. A profiler shows the time in the loop, not in any work.

## Cause

The consumer of a queue that returns false when empty is written as a loop:

```cpp
while (!queue.try_pop(item)) { std::this_thread::yield(); }
```

It works, and it is a thread spending its whole share of the machine to find out
that nothing happened.

Measured once, on a 12 core machine with gcc 11, a consumer waiting 100 ms for one
item:

| how it waited | wakeups | CPU used, of 100 ms |
|---|---|---|
| spin, with `yield` | 231,000 to 309,000 | **100.05 ms** |
| poll every 1 ms | 92 | 1.85 ms |
| condition variable | **1** | **0.04 ms** |

Polling every millisecond is cheap, and still wakes once a millisecond for as long
as nothing happens, and an item arriving just after a poll waits for the next one.
The suite asserts the counts, since time depends on the machine that day: the waiter
went to sleep once and the poller asked more than a hundred times.

## Fix

Sleep until there is something to do:

```cpp
#include <rc/rt/blocking_queue.hpp>

rc::rt::BlockingQueue<Reading> queue;
Reading reading;
using Queue = rc::rt::BlockingQueue<Reading>;
if (queue.pop_for(reading, std::chrono::milliseconds(100)) == Queue::Result::kGot) {
  // use it
}
```

Give every wait a deadline. A control path that waits for ever on a producer that has
died has stopped controlling.

A lock free queue, lesson 07-02, is still the right thing where a producer must never
block. The two answer different questions: that one never makes the producer wait,
this one lets the consumer sleep.
