# The Loop, Measured: Three Ways to Wait, and What Each Costs in Latency and CPU

> A consumer that spins was a thousand times faster than one that sleeps, and used a
> whole core to do it.

**Type:** Build
**Time:** about 130 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 07-02, 07-03, 07-06, 07-07, 07-08, 07-14

## The Problem

Phase 07 ends with a number: how long, from a sensor producing a reading to a controller having
it, and how that number is distributed. Every earlier lesson built a piece of it. This wires the
pieces into one rig and measures it, and the measurement is the lesson.

```
a producer thread       paced on a fixed grid (07-08), stamping each reading with the time
a queue between         a lock free one (07-02) or a blocking one (07-06)
a consumer thread       takes each reading and records now minus the stamp in a histogram (07-03)
a stop token            asks both to finish (07-14), and both are joined (07-07)
```

## The Concept

### Latency is from the stamp

The time is taken when the reading is **made**, and carried with it:

```cpp
const Reading reading{now_ns()};      // in the producer
latency.record(static_cast<double>(now_ns() - reading.stamp_ns) / 1000.0);   // in the consumer
```

A loop-time measurement, the time between two readings being looked at, would read 5000
microseconds at 200 Hz for every design below. It measures the loop and not how long a reading
waited. `E-RT-0012`.

### Three ways to wait

Measured on gcc 11, a 200 Hz producer for 2 seconds, 400 readings, run twice. The consumer waiting
three ways, with its own thread's CPU time:

| consumer | p50 | p99 | max | consumer CPU |
| --- | --- | --- | --- | --- |
| spins, with `yield` | 0.4 us | 1.3 to 1.6 us | 12 to 50 us | **1999 ms of 2000** |
| polls every 1 ms | 514 to 549 us | 1080 to 1146 us | 1087 to 1254 us | 31 to 32 ms |
| sleeps on a condition | 39 to 65 us | 167 to 245 us | 473 to 1548 us | 4.7 to 8.9 ms |

Nothing is free.

- **Spinning** is a thousand times faster at the median and uses a whole core to do it. Lesson
  07-06's finding, in a different light. `E-RT-0014`.
- **Polling** is cheap and adds about half the poll interval to every reading.
- **Sleeping on a condition** costs a few tens of microseconds of wakeup and almost no CPU, and has
  the worst tail: one reading took 1.5 ms, which is the scheduler and not the queue.

Which is right depends on the budget. A 200 Hz loop has 5000 microseconds per cycle. Sleeping's p99
of 245 is 5 percent of it. Polling's 1100 is 22 percent. Spinning's 1.6 is nothing, and needs a core
that nothing else may have. A 1 kHz loop with a 1000 microsecond budget cannot afford to poll at 1 ms
at all, and a robot with four cores and a vision pipeline cannot afford to spin.

### The numbers are only as good as the books

What the suite asserts is not those figures, which belong to the machine that day. It asserts the laws
that make any such measurement trustworthy.

**Conservation: every reading is delivered or dropped, and they add up.**

```
    produced 60, delivered 19, dropped 41, slots missed by the producer 0
```

A 5 ms producer against a consumer that takes 20 ms a reading, with a queue of 4. **41 of 60 readings
were lost.** The 19 that got through were taken within about four readings of being made, because the
queue never held more than four: a respectable latency distribution for a consumer throwing away two
thirds of its input. A latency number without its conservation law is a number about whatever got
through. `E-RT-0013`.

**Slots: readings produced plus slots the pacer dropped is the number of cycles in the run.** The same
slow consumer left the producer's rate alone, 60 produced and 0 missed. A producer that blocks on a full
queue has moved the loss from the queue to the sensor.

**Order: the percentiles are ordered,** and the worst is at least the median, with the one exception
that the percentile is the top edge of a one microsecond bucket, so the true worst can sit just below it.

**Stop: a request ends the run promptly with the books still balanced.** A 5000 ms run with a stop
requested after 80 ms returned after 80.

### What the suite does not assert, and why

An earlier draft asserted that a spinning consumer went round more than 100 times per reading. It
failed in **3 of 10** runs with all twelve cores busy. A spinning thread is descheduled like any other, so
under load it goes round far less. A count that depends on the CPU the scheduler grants is not a test.

It is printed, and not asserted. What is asserted is what a slower machine cannot break: a poller sleeps at
least a millisecond between empty rounds, so in 300 ms it cannot go round more than about 300 times plus
once per reading, and a sleeper goes round once per reading plus a timeout every 50 ms. After the change:
0 failures in 12 quiet runs and 0 in 20 with every core busy.

## Build It

One function, `run_rig`, and every piece in it is one you built.

Set up the queue for the style, a histogram with 1 us buckets and 20000 of them, a `producer_done` flag, a
`StopSource`, and a `StopCallback` on the caller's token that requests it. The producer is a `ScopedThread`
with a `Pacer`: while stop is not requested and the run has not lasted its duration, stamp a reading, push it
(counting a refusal as dropped), and sleep for what the pacer says. At the end it records the pacer's missed
slots, sets `producer_done`, and closes the blocking queue.

The consumer is a `ScopedThread` that takes each reading, does its work if asked, and records the latency.
Spin: `yield` when empty. Poll: sleep 1 ms. Sleep: `pop_for` with a deadline. When the producer is done and the
queue is empty, look once more and stop.

Both are joined before anything is read, which the closing brace does. The stub returns an empty report,
which breaks every law.

## Use It

```cpp
#include <rc/rt/latency_rig.hpp>

rc::rt::RigOptions options;
options.style = rc::rt::WaitStyle::kSleep;
options.rate_hz = 200;
const rc::rt::Report report = rc::rt::run_rig(options);
std::cout << report.p50_us << " " << report.p99_us << " " << report.max_us << " dropped " << report.dropped << "\n";
```

Change one thing at a time and read the drop count before the percentiles. Then put your own consumer in place
of the rig's, and keep the stamp.

Run it under the thread sanitizer with clang: it reported nothing in 8 runs. GCC 11's reports the timed waits
falsely (`E-TSAN-0002`).

Two limits.

**It is one producer and one consumer, on one machine.** The clocks are the same clock, which is what makes the
subtraction meaningful. Across machines the clocks have to be synchronised first.

**The scheduler is in the numbers.** The sleeper's worst of 1.5 ms is not the queue's. Lesson 07-15 is what the
scheduler does with priorities, and it is the next knob to turn.

## What Breaks First

`E-RT-0012` is the latency measured from the wrong end, which measured the loop and not the reading.

`E-RT-0013` is the report that only counted the readings that got through.

`E-RT-0014` is the consumer that spun to win latency and took the machine.

## Ship It

`rc/rt/latency_rig.hpp`, in `namespace rc::rt`.

This closes phase 07. A threaded design in this curriculum from here on states its latency as a distribution
from a stamp, its drops as a count, and its CPU as a cost.
