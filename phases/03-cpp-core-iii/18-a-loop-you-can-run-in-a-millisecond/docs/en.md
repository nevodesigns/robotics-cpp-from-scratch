# A Loop You Can Run in a Millisecond: A Scheduler on a Test Clock, and Why Its Time Is an Integer

> Phase 03 gave you a clock you can control and a queue that gives the earliest first. Joined, they run ten minutes
> of a control loop in a fraction of a second, and the ways it can go wrong are about time and order.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 02-08, 03-05, 03-16

## The Problem

A robot runs several things at different rates: a sensor at 100 Hz, a logger at 10 Hz, a command a second. To test how they behave together you can run the real thing and wait, which takes the time it takes and varies from run to run, or you can run the same logic against a clock you control.

Lesson 03-05 made the clock injectable, so the code takes time as an argument. Lesson 03-16 made a queue that gives the earliest event. A scheduler joins them: ask it to run until a time, and it takes the next event, sets the clock to its time, runs it and takes the next. Nothing waits.

## The Concept

### The loop

```cpp
while (!queue_.empty() && time_of(queue_.top()) <= end) {
  Event e = queue_.top();  queue_.pop();
  clock_->set(time_of(e));
  task(time_of(e));
  if (periodic) push(time + period);
}
clock_->set(end);
```

The clock reads the event's time while the task runs, and sits at `end` afterwards. A task due at 250 ms did not run in a run to 100 ms, and the clock stood at 100 ms; the next run to one second ran it, with the clock reading 250 ms inside it.

### Ten seconds of two rates

A 100 Hz sensor and a 10 Hz logger over ten seconds, the start and the end both being ticks: the sensor ran **1001** times, the logger **101**, **1102** events in all. Ten minutes of a 1 kHz loop was **600001** ticks and time went backwards **0** times, and no test waited for any of it.

### Time is an integer

A tick every tenth of a second up to 0.3 seconds is four ticks: 0, 0.1, 0.2 and 0.3. With whole nanoseconds it ran **4** times. With doubles added up it ran **3**, because 0.1 is not exact and three of them make 0.30000000000000004, which is later than 0.3.

The scheduler keeps `Nanoseconds`, an `int64`, as lesson 03-05 did. The queue's event time is a double, which holds whole nanoseconds exactly up to about 104 days, so the conversion in and out loses nothing. See `E-TIME-0005`.

### Ties go by registration order

When the sensor and the logger are both due at 100 ms, one has to go first. The first version of this scheduler broke ties with the queue's order, which is the order events were put in. A repeating task queues its next tick as it finishes the last, so the slower task's event, waiting since long
before, won: the suite saw **logger first, then sensor**, though the sensor was registered first. The logger recorded the previous sample.

The fix is to break ties by the task's index, which is the order it was registered. See `E-TIME-0006`.

### Stopping and carrying on

A 7 ms task from 3 ms, run to ten seconds: **1429** events in one run and **1429** in a hundred runs of a tenth of a second each. A run can be resumed and gives the same events.

### A task that adds a task

A task registered another at +5 ms and another at +500 ms. The first ran in the same run, at 15 ms; the second waited for the next, and ran at 510 ms. The scheduler copies the entry's task out of its vector before calling it, because adding an entry can move the vector and the function being run with it. See `E-TIME-0007`.

## Build It

`every` and `run_until` in `exercise/solution.hpp`. `at`, `push` and the data are given.

```
rcpp verify 03-18
```

## Use It

`rc/core/scheduler.hpp` joins `rc::core`.

```cpp
TestClock clock;
Scheduler s(&clock);
s.every(10'000'000, 0, [&](Nanoseconds now) { sensor.sample(now); });
s.every(100'000'000, 0, [&](Nanoseconds now) { logger.write(now); });
s.run_until(10'000'000'000);
```

**What this does not do.** It runs one thing at a time in time order, so it cannot find a race between two threads: it is a model of when things happen and not of how they overlap. A task takes no time here, whereas a real one does, and a loop that overruns its period cannot be
shown until a task can advance the clock itself (lesson 07-08 measures that on a real one). It has no way to cancel a task. And the times are exact in the model, where a real clock has jitter, which a test should add on purpose.

## What Breaks First

- **Time kept as doubles.** A tick lost. See `E-TIME-0005`.
- **Ties in queue order.** The wrong task first. See `E-TIME-0006`.
- **A reference into the task list across a call.** See `E-TIME-0007`.

## Ship It

`rc/core/scheduler.hpp` holds `Scheduler`.

That finishes phase 03's ground: a clock you control, a queue of what comes next and a scheduler that joins them, so dt has a provenance and a test can make ten minutes pass in a blink.
