# Two Counters, One Cache Line: Sharing You Did Not Ask For

> Both fixes were obviously right and neither one showed up in the measurement.
> Together they nearly doubled it.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 07-04, 07-02

## The Problem

Two threads. Two counters. Nothing shared between them: no lock, no queue, no
common variable. Adding the second thread makes the program six times slower.

Nothing in the source says why, because the thing that is shared is not in the
source. It is the 64 bytes the hardware moves at a time.

## The Concept

### Caches move lines, not bytes

A processor does not fetch the eight bytes you asked for. It fetches the 64-byte
line containing them, and when another core writes anywhere in that line, this
core's copy is thrown away.

So two variables in one line are one variable as far as the hardware is
concerned, whatever your program thinks. Two atomic counters, two threads, one
and a half million increments each, best of five runs:

| bytes between them | ms | relative |
|---|---|---|
| 8 | 48.8 | 1.00 |
| 16 | 55.2 | 1.13 |
| **64** | **15.2** | **0.31** |
| 128 | 9.5 | 0.20 |

Nothing changes between 8 and 16, because both are inside one line. Crossing the
boundary changes everything, and going further buys nothing more. It is a cliff,
not a slope.

### The most natural code writes the bug

Nobody sets out to put two counters eight bytes apart. They write this:

```cpp
std::vector<std::atomic<long>> counts(threads);
// ... each thread increments counts[my_index]
```

which is the obvious, correct-looking way to give every thread its own slot, and
which puts eight slots in one line.

| four threads, 1.5 million increments each | ms | relative |
|---|---|---|
| a vector of atomics, one per thread | 60.5 | 1.00 |
| the same, one slot per cache line | 10.2 | 0.17 |
| a local on the stack, published once | 2.9 | 0.05 |

Read the last row before the middle one. **Not sharing at all beats sharing
carefully**, by three times again, and it is usually available: accumulate into
a local and write the result once when the thread finishes. The stack is per
thread by construction.

Padding is for the values that genuinely have to be shared while they change.

### Padding the type, not the struct

```cpp
struct Slot {
  std::atomic<long> value;
  char padding[56];              // 64 bytes, and no alignment requirement
};
```

That is 64 bytes and can start anywhere, so in a `std::vector<Slot>` the first
element may begin part way through a line and two `value` members can still land
together.

```cpp
template <class T>
struct alignas(kCacheLine) Padded {
  T value{};
};
```

This one has a size **and** an alignment of 64, so every element of a vector
starts on a line boundary. `sizeof` tells you how far apart two elements are and
nothing about where the first one starts; both matter, and only `alignas` on the
type gives you both.

The test checks the distance between two elements of a real vector rather than
reading the declaration, because that is where the padding was supposed to apply
and where it goes missing.

### Two fixes, neither of which works alone

Now the queue from lesson 07-02. Its two indices are adjacent, so the obvious
change is to separate them. The other obvious change is to stop reading the
other thread's index on every operation and keep a cached copy.

Both are right. Over several runs of a million items through a 1024 slot queue:

| change | worth |
|---|---|
| a line apart, index read every time | 1 to 21 percent |
| adjacent, index cached | -8 to +14 percent |
| **both** | **74 to 218 percent** |

Each alone hovers around the run-to-run noise. Together they at least double it
and sometimes do a great deal better.

A word on how those were measured, because it matters here. Each figure is the
**smallest** of several runs rather than the mean of them. Noise on a shared
machine can only ever add time, so the minimum is the closest any run came to
measuring the thing itself, and a single sample measures whatever else the
machine happened to be doing.

Even so, **none of the timing in this lesson is asserted**. Every table here is
reported and checked by a person, and the tests gate only on what does not
depend on what else the machine is doing: that `Padded` really is a line wide
and a line apart in a vector, and that the queue is still a queue.

That was not the original plan. The queue table failed about one build in three
on a shared runner with two virtual cores and a sanitizer attached, and then the
counter table, which had held on three lanes, **inverted** on a Windows runner.

A shared virtual machine cannot measure cache behaviour. It has neighbours, its
cores are not yours, and its scheduler moves your threads between them. That is
not a flaw in the measurement, it is a fact about where it was taken, and it is
the reason real-time work is measured on the target rather than on a build
server. Reproduce these numbers on hardware you can see.

A gate that fails when nothing is wrong teaches people to rerun the build, which
is worse than not gating at all.

The reason is that there were two ways the line kept moving:

- **Padded but not cached.** The producer still reads the consumer's index on
  every push, to see whether there is room, so it still touches the consumer's
  line every time.
- **Cached but not padded.** The cached copy sits in the same line as the index
  the other thread writes, so it is invalidated about as often as the real one
  would have been read.

Remove one cause and the other keeps the symptom exactly where it was.

```cpp
if (next == cached_head_) {
  cached_head_ = head_.value.load(std::memory_order_acquire);
  if (next == cached_head_) return false;
}
```

Consult the cached copy; go and look only when it says stop; believe the fresh
value if it says stop too. Then place each cached copy beside the index its own
thread writes, so the producer touches `tail_` and `cached_head_`, the consumer
touches `head_` and `cached_tail_`, and the two sets never meet.

**The general lesson is worth more than the queue.** When a fix that is
obviously correct produces no measurable improvement, the usual reason is not
that the fix was wrong. It is that something else is producing the same effect.
Measure the combinations, not the changes.

### The constant

C++17 named this distance `std::hardware_destructive_interference_size`. The
standard library on Ubuntu 22.04 does not provide it, which is a fair summary of
how portable that answer is.

So `kCacheLine` is a plain 64 with a measurement behind it, and the suite sweeps
the distance and prints where the interference stops rather than trusting either
the constant or the standard.

## Build It

Implement `Padded`, and the two halves of the queue that consult a cached index,
in `exercise/solution.hpp`.

```
rcpp verify 07-05
```

The suite checks that the padding survives a vector, sweeps two counters apart
by one byte at a time until they stop interfering, contends four threads over
one array, and then runs the queue with each of the two changes on and off.

## Use It

**Accumulate locally and publish once**, wherever the shape of the work allows
it. It is the fastest option measured here and it needs no new type.

**Pad only what more than one thread writes while it is changing.** A padded
counter is 64 bytes instead of 8; for a handful of values that is nothing and
for a million objects it is a different problem.

**Check the distance in the container**, not `sizeof` in the declaration.

**Measure combinations.** This lesson would have concluded that padding does not
help, from a measurement that was correct.

**Take the minimum of several runs**, not the average, and say how many.

**Know which measurements can carry a gate.** A cache measurement on a shared
virtual machine is not one of them, whatever its size on your desktop. Gate on
layout and behaviour; report timing and read it.

## What Breaks First

- **Two hot variables sharing a line.** See `E-RT-0005`.
- **Half a fix, which is no fix.** See `E-RT-0006`.
- **Padding that does not survive a container.** See `E-RT-0007`.

## Ship It

`kCacheLine` and `Padded` join `rc::rt` as `rc/rt/cache_line.hpp`, and
`rc/rt/spsc_queue.hpp` from lesson 07-02 gains both changes: its indices now sit
on separate lines with a cached copy of the other beside each. The queue's
behaviour is identical and its tests are unchanged, which is the point. It was
already correct. It is now about twice as fast.
