# A Queue Without a Lock, and What Acquire and Release Actually Mean

> The control loop has a millisecond. A mutex cannot promise it anything, because how long you wait depends on what the other thread is doing.

**Type:** Build
**Time:** about 135 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 07-01

## The Problem

The last lesson ended with a working mutex and a warning. A mutex is correct, and
its cost is unbounded from the waiting thread's point of view: you wait for as
long as the holder takes, and the holder might be descheduled halfway through
while still holding it.

For a value published now and again that is fine. For the path between a sensor
thread and a control loop with a deadline it is not, and the failure is the worst
kind: the system works for hours and then misses a cycle at the moment it is
loaded.

The structure that goes there is a queue with no lock at all, and building one
correctly needs the one part of C++ this curriculum has avoided so far: what
`std::atomic` actually promises about ordering.

## The Concept

### One producer, one consumer, and why that constraint is the whole trick

A general lock free queue, with any number of threads pushing and popping, is
genuinely hard and easy to get subtly wrong.

A queue with **exactly one** producer thread and **exactly one** consumer thread
is about forty lines. That restriction is not a compromise, it is the design:
each index is written by exactly one thread, so no index is ever contended, and
almost all of the difficulty disappears.

It is also the shape robotics actually needs. One sensor thread produces, one
control thread consumes. When you need more, you use more queues.

### The ring, and the wasted slot

Storage is a fixed array. The producer writes at `tail` and advances it; the
consumer reads at `head` and advances it; both wrap around at the end.

That leaves one question: `head == tail` means empty, but a completely full
buffer also brings them together. Two answers exist, and this lesson takes the
simpler:

**Keep one slot empty.** Allocate one more than the capacity, and treat the queue
as full when advancing `tail` would land on `head`. One wasted element buys an
unambiguous distinction with no extra state and no extra synchronisation.

The alternative, a separate count, is another shared variable and therefore
another ordering problem. The wasted slot is cheaper than the count.

### What acquire and release actually mean

Here is the part worth reading twice.

An atomic guarantees the value itself is never seen half written. That is not
enough. The consumer must also be guaranteed to see the **reading** the producer
wrote before it advanced the index, and nothing about writing an index says
anything about a different variable.

That is what the ordering arguments are for.

- **Release** on a store means: everything this thread wrote before this store is
  visible to any thread that acquires this same variable and sees this value.
- **Acquire** on a load means: everything the writing thread did before its
  release is now visible to me.

They work as a pair, and together they carry the data across:

```cpp
// producer
buffer_[tail] = reading;                       // ordinary write
tail_.store(next, std::memory_order_release);  // publishes it

// consumer
const auto tail = tail_.load(std::memory_order_acquire);   // sees the write above
out = buffer_[head];
```

Use `relaxed` on both and the atomic index is still atomic, and the reading it
refers to may not be visible yet. The consumer reads a slot the producer has not
finished writing. That is the same torn read as the last lesson, arriving by a
subtler route, and it is the mistake that makes lock free code frightening.

Each thread may load **its own** index relaxed, because nobody else writes it.

### The cost, measured, including the part that disappoints

A mutex queue and this one, half a million readings pushed between two threads,
nine runs on this machine. Typical figures:

```text
                 median push     99th percentile        worst push
mutex queue         100 ns            1150 ns        25000 to 486000 ns
lock free queue      32 ns             125 ns        17000 to 128000 ns
```

The median is about three times better and the ninety ninth percentile about
nine times better. Those two numbers are stable across runs and they are the
honest case for the structure.

**The worst case is not the clean win the folklore promises.** Lock free code is
widely described as giving bounded latency, and this measurement does not support
that. The worst push was usually better than the mutex and occasionally about
the same, and both wander by an order of magnitude between runs.

The reason is that the worst case here is not caused by the lock at all. It is
the operating system descheduling the measuring thread, and no data structure can
prevent that. Removing the lock removes **lock induced** blocking; scheduler
induced blocking is untouched.

That is worth carrying, because it is where the phrase real time actually earns
its keep. A bound on the worst case needs a kernel that will not deschedule the
thread arbitrarily, a core reserved for it, and a priority that means something,
which is a later lesson in this phase. A lock free queue is a necessary part of
that and nowhere near sufficient on its own.

What this structure genuinely buys, and it is worth having, is that a thread can
never be blocked because another thread holding a lock was descheduled. That is
a real failure mode a mutex has and this does not.

### What lock free does not mean

It does not mean wait free. It does not mean fast in every case. And, as the
measurement above shows, on an ordinary kernel it does not mean a bounded worst
case either.

It means that no thread can be blocked because another thread holding a lock was
descheduled. That is a genuine failure mode a mutex has and this does not, and
it is a smaller claim than the one usually made for it.

It is also not a licence to use it everywhere. Lock free code is harder to read,
harder to review, and easy to get wrong in ways that testing does not catch. Use
it on the path with a deadline and a mutex everywhere else.

## Build It

Implement `SpscQueue` in `exercise/solution.hpp`:

- The constructor allocates once and never again.
- `push(reading)` returns false when full rather than blocking or growing.
- `pop(out)` returns false when empty.
- `size()` and `capacity()`.
- Ordering: release when publishing an index, acquire when reading the other
  thread's index, relaxed for your own.

```
rcpp verify 07-02
```

One test runs a producer and a consumer exchanging two hundred thousand readings
and requires every one to arrive, in order, with none lost or duplicated.

## Use It

Boost has `lockfree::spsc_queue`, and most robotics middlewares have something
similar inside them. Use one rather than writing your own where you can: this
lesson exists so that you can read theirs and know what the ordering arguments
are doing.

For more than one producer you need a different structure, and that is where the
difficulty genuinely lives. The honest answer for most robots is more queues
rather than a cleverer queue.

Run this under the thread sanitizer. Lock free code is exactly where a race
hides from testing, and the previous lesson measured how badly.

## What Breaks First

- **A consumer reading a slot the producer has not finished writing.** Relaxed
  ordering where release and acquire were needed. See `E-THREAD-0003`.
- **A queue that reports full when empty, or loses one element.** The wasted slot
  and the wrap arithmetic. See `E-THREAD-0004`.
- **An index that runs past the end of the buffer.** The modulus is against the
  storage size, which is capacity plus one. See `E-CPP-0007`.

## Ship It

`SpscQueue` joins `rc::rt` and is the structure on the deadline path for the rest
of the curriculum: sensor thread to control loop in phase 15, and the audio and
image buffers wherever a producer runs faster than a consumer.
