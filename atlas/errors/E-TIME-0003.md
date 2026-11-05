id: E-TIME-0003
title: A loop that runs slower than its period, because it sleeps after the work
match: (work plus the period|runs at 8[0-9] Hz|slower than its period|sleep_for\(period\) after the work)
platforms: linux, windows
teaches: 07-08-a-loop-that-holds-its-rate
---

## Symptom

A control loop meant to run at 100 Hz runs at 83. Nothing errors. A filter tuned for
a 10 ms step is wrong by a fifth, a velocity estimate reads low, a PID with an integral
winds up at the wrong rate. The loop "works" and every number derived from its timing
is off by a constant factor.

## Cause

```cpp
while (running) { work(); std::this_thread::sleep_for(period); }
```

Each cycle takes the work **plus** the period. With 2 ms of work in a 10 ms period
that is 12 ms.

Measured. Simulated, exactly: the 100th cycle started at 1188 ms where the grid puts
it at 990, an interval of 12 ms and 83.3 Hz. On real threads and a real clock, gcc 11,
a 100 cycle run wanting 100 Hz:

| | total | rate |
|---|---|---|
| `sleep_for(period)` after the work | 1210 ms | **82.6 Hz** |
| `sleep_until` a fixed grid | 1000 ms | 99.99 Hz |

The tempting repair, sleeping for what is left of the period, still drifts. 300 cycles
of `sleep_for(period - work)` came out at 3032 ms, **105 to 110 us late per cycle**.
Every sleep overshoots a little, as every sleep does, and nothing corrects it, so the
error is added up. The grid was about 1 us late per cycle.

## Fix

Sleep until a **deadline computed from the start**, not for an amount computed from the
last result:

```cpp
#include <rc/rt/pacer.hpp>

rc::rt::Pacer pacer(period_ns, now_ns());
while (running) {
  work();
  std::this_thread::sleep_for(std::chrono::nanoseconds(pacer.sleep_needed(now_ns())));
}
```

The pacer asks where the next point on the grid is. The error of one cycle is gone by
the next instead of being kept.

Measure the rate you got as well as the rate you asked for. Lesson 07-03's `LoopMonitor`
is how.
