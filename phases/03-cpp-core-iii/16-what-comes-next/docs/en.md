# What Comes Next: A Queue That Gives the Earliest First, Ties in Order, and a Window That Drops From the Front

> A simulation has things to do at times, and a filter has readings to forget. Two containers lesson 03-03 did not
> cover do these, and each has a default that does the wrong thing quietly.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 03-03, 03-04

## The Problem

A simulated robot has things that must happen at certain times: a sensor sample at 10 ms, a command at 12 ms, a fault injected at 12 ms as well. They are scheduled in any order, and must be run earliest first. And a filter keeps the last thousand readings and forgets the oldest as each new one arrives.

Lesson 03-03 gave the sequence containers and the map. The two for these jobs are a priority queue and a deque. Each does something unexpected on first use.

## The Concept

### A priority queue gives the largest

`std::priority_queue` is a max-heap: `top()` is the greatest element by its comparison, and the default comparison is `<`. The numbers 5, 1, 9 and 3 in a plain `priority_queue<int>` gave **9** first. A schedule wants the earliest, so the comparison has to say "comes after":

```cpp
struct Later {
  bool operator()(const Event& a, const Event& b) const { return a.time > b.time; }
};
```

The queue then gives the one that is not later than any other. The event queue gave 1, 3, 5, 9. See `E-CPP-0094`.

### Ties need a rule

A priority queue orders elements that differ. For two that compare equal it promises nothing. Eight events at the same time, scheduled 0 to 7:

| queue | order out |
|---|---|
| the event queue | 0 1 2 3 4 5 6 7 |
| `priority_queue` comparing only the time, libstdc++ | 0 2 6 5 7 4 1 3 |

The second is not promised on any library, and it can change with an unrelated edit. Each event gets a sequence number when scheduled, and the comparison uses it after the time. Ten thousand events over only 500 different times, with plenty of ties, came out exactly sorted with ties in order. See `E-CPP-0095`.

### A window forgets from the front

`Window` keeps the last N readings and their mean. The oldest leaves from the front as the newest arrives at the back. A vector must move every element behind the first one forward to remove it. A deque does not:

| window of 1000, 5000 readings | elements moved |
|---|---|
| `erase` at the front of a vector | 4000000 |
| `pop_front` on a deque | 0 |

The time of the vector version grows with the window. See `E-CPP-0096`.

The mean is kept as a running sum: add the new reading, take away the one that left. After a thousand more readings of 7.0 the mean was within 1e-9 of 7.0, so the running sum did not drift. A pass over the window for every reading would cost as much as the erase.

## Build It

`EventQueue` and `Window` in `exercise/solution.hpp`. `Event` and the `Later` comparison are given.

```
rcpp verify 03-16
```

## Use It

`rc/core/event_queue.hpp` joins `rc::core`.

```cpp
EventQueue q;
q.schedule(0.010, kSampleSensor);
q.schedule(0.012, kSendCommand);
while (!q.empty()) { Event e = q.pop(); /* run it */ }
```

**What this does not do.** You cannot cancel or reschedule an event once it is in, since a priority queue only gives its top. A simulation that needs it marks the event cancelled and skips it when it comes out. The sequence number is an `unsigned long`, which is far more than a run
needs and wraps in theory. `Window` keeps a floating point running sum, so over a very long run its error can grow and it may want recomputing now and then. And a deque is slower than a vector to iterate, because its elements are in blocks and not in one line.

## What Breaks First

- **A plain `priority_queue` for a schedule.** Latest first. See `E-CPP-0094`.
- **Ties with no rule.** An order nobody chose. See `E-CPP-0095`.
- **A window in a vector.** Every drop moves the rest. See `E-CPP-0096`.

## Ship It

`rc/core/event_queue.hpp` holds `Event`, `Later`, `EventQueue` and `Window`.

Lesson 03-05 gave the simulation a clock. This gives it something to do with one.
