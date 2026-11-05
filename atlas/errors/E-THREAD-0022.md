id: E-THREAD-0022
title: A worker that took milliseconds to stop, because it polled a flag between sleeps
match: (took milliseconds to stop|polled a flag between sleeps|slow to notice the stop|shutdown takes as long as the poll)
platforms: linux, windows
teaches: 07-14-asking-work-to-stop
---

## Symptom

Shutdown works and takes noticeably long. A robot asked to stop keeps moving for a few
milliseconds. A test suite whose teardown waits on worker threads is slow in proportion to the
number of workers, and each wait is some fraction of a poll interval.

## Cause

A worker that checks a flag between sleeps notices the stop only when it next wakes:

```cpp
while (!stop.load()) std::this_thread::sleep_for(std::chrono::milliseconds(10));
```

Measured on gcc 11, 40 trials, how long after the request the worker had stopped:

| how it waited | median | worst |
|---|---|---|
| a flag polled between 10 ms sleeps | **3.27 ms** | **9.3 ms** |
| a wait the request itself wakes | **72 us** | 105 to 122 us |

Polling is not wrong, it is slow, in proportion to its sleep: the expected delay is half the interval and
the worst is all of it. Shortening the sleep trades latency for wakeups, and a worker that checks every
millisecond to stop within one is a worker that wakes a thousand times a second to do nothing, which is
`E-THREAD-0006`.

## Fix

Make the request do the waking. Wait on a condition that the request signals, and let a callback signal it:

```cpp
#include <rc/rt/stop.hpp>

if (rc::rt::sleep_unless_stopped(token, std::chrono::milliseconds(500))) return;   // woken by the request
```

About 45 times quicker at the median and 80 at the worst, with no polling at all.

Keep a bounded wait as well, as in lesson 07-06. A consumer that wakes at least occasionally notices a
stop request even if a notification is somehow missed.
