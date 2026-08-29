# Containers: Choosing One, and What It Costs

> A robot that allocates memory inside its control loop is a robot that misses a deadline eventually, on a schedule nobody can predict.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 03-02

## The Problem

A robot records sensor readings for as long as it runs. Days, in a warehouse.

Push each reading onto a `std::vector` and the program grows until it is killed.
Push onto a `std::map` keyed by timestamp and it grows in a more organised way
and is still killed. Keep the last ten thousand by erasing the front of a vector
and every reading copies the entire buffer down by one.

Meanwhile the control loop runs at a kilohertz and must not pause. A container
that reallocates at an unpredictable moment introduces exactly the pause it
cannot have.

So: which container, and what does it cost?

## The Concept

### The four you will actually use

| Container | Holds | Find by key | Insert at end | Contiguous |
|---|---|---|---|---|
| `std::vector` | a growable sequence | scan, linear | amortised constant | yes |
| `std::array` | a fixed size sequence | scan, linear | not possible | yes |
| `std::map` | sorted key to value | logarithmic | logarithmic | no |
| `std::unordered_map` | key to value | constant on average | constant on average | no |

`std::vector` is the default and should be the default. Contiguous memory is
what modern processors are fast at: reading one element pulls its neighbours
into cache, so a linear scan of a vector routinely beats a cleverer structure
that scatters its elements across memory. For a few dozen items, scanning a
vector beats a map even for lookup.

Reach for `std::map` when you need the keys in order, and `std::unordered_map`
when you need lookup by key and do not care about order. The difference in
practice: iterating a `map` gives sorted keys for free, which is exactly what a
status report wants, and iterating an `unordered_map` gives an order that may
change between runs, which makes output that is hard to compare.

### Growth, and why it pauses

A vector holds a block of memory with room for a certain number of elements,
its **capacity**. Pushing past that allocates a bigger block, moves everything
across, and frees the old one.

The cost is amortised constant, which is a statement about the average. The
individual push that reallocates is not constant, and it happens at a moment
determined by the history of the container rather than by anything happening
now. That is precisely the property a control loop cannot accept.

Two answers, and robotics uses both.

`reserve` the capacity up front when you know roughly how many there will be.
Growth then never happens during the part that matters.

Or use a **fixed capacity buffer that never grows**, which is what this lesson
builds. A ring buffer holds the last N items in a block allocated once, and
writing item N+1 overwrites the oldest. No allocation, no move, no pause, and
memory that is bounded for a machine that runs for a month.

### Iterator and reference invalidation

This is the container rule that produces the strangest bugs.

When a vector reallocates, every pointer, reference and iterator into it becomes
dangling. Code that looks obviously correct is not:

```cpp
double& first = readings[0];
readings.push_back(1.0);   // may reallocate
first = 2.0;               // may write to freed memory
```

Nothing warns. It works until the push happens to reallocate, which depends on
the capacity, which depends on how many readings arrived earlier.

`std::map` and `std::unordered_map` are kinder: references to elements stay
valid across inserts, though `unordered_map` invalidates iterators when it
rehashes. The rule worth carrying: **do not hold a reference into a container
across an operation that could change its size.**

This is the same lifetime question as lesson 02-01, wearing different clothes.

### Reading the cost table honestly

Complexity notation describes how cost grows, not how fast something is. A
logarithmic lookup in a `map` involves following pointers to scattered nodes,
and a linear scan of a small vector touches memory the processor has already
fetched. For small collections the vector usually wins despite the worse
notation.

The honest rule: start with a vector, and change only when a measurement says
to.

## Build It

Implement `SensorLog` in `exercise/solution.hpp`. It is what every later phase
records into.

- `record(name, value, at_ms)` stores the reading as the latest for that sensor
  and appends it to a bounded history.
- The history holds at most `capacity` readings. Once full, a new reading
  overwrites the oldest, and no allocation happens.
- `latest(name)` returns the most recent reading for a sensor, or nothing.
- `names()` returns every sensor name seen, in sorted order.
- `history()` returns the readings held, oldest first.
- `size()` and `capacity()` report the obvious things.

```
rcpp verify 03-03
```

One test records a hundred thousand readings into a log with capacity sixteen
and requires that the memory in use never grew, using the leak counting harness
from phase 02.

## Use It

`std::deque` is the standard container closest to a ring buffer, and it does not
give the guarantee that matters here, which is a fixed allocation. Boost and
several embedded libraries ship a `circular_buffer`; if one is available, use it.

The reason to write it once is that this is the shape of nearly every buffer in
a robot: the last N poses, the last N frames, the last second of audio. Phase 07
uses one to hand samples between threads without allocating, and phase 15 uses
one for every sensor stream.

## What Breaks First

- **A reference into a container used after the container grew.** Nothing warns,
  and it works until it does not. See `E-CPP-0016`.
- **An index computed with a modulus that goes out of range.** Ring buffer
  arithmetic is where off by one lives. See `E-CPP-0007`.
- **A view outliving the buffer it viewed.** Same lifetime question as pointers.
  See `E-MEM-0001`.

## Ship It

`SensorLog` joins `rc::core` and every later phase records into it. The ring
buffer inside it is the artifact that matters, because a bounded buffer with no
allocation is what makes a control loop's timing predictable.
