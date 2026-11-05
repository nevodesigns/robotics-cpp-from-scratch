id: E-TIME-0004
title: A loop that caught up after a stall with a burst of tiny steps
match: (burst of updates|runs back to back to catch up|dt far below the period|catch up with a burst)
platforms: linux, windows
teaches: 07-08-a-loop-that-holds-its-rate
---

## Symptom

A robot is fine, hiccups for a moment, and then lurches. A derivative term spikes. A
filter jumps. The log shows a gap of a few tens of milliseconds followed by several
updates a couple of milliseconds apart, and then normal again. The total time is right,
which is why a rate counter says nothing is wrong.

## Cause

A fixed grid is the right idea, and on its own it has a failure. When a cycle overruns,
the next deadlines are already in the past, so there is no sleep, and the loop runs the
missed cycles one after another until it has caught up.

Measured on real threads, gcc 11: one 45 ms cycle in a 10 ms loop. A plain fixed
deadline started **5 cycles late**, back to back with no sleep between them, and the total
time came out at 300.06 ms against an ideal 300.

Simulated, exactly, 20 cycles with that stall:

| | shortest interval | intervals shorter than the period |
|---|---|---|
| a plain fixed grid | **2 ms** | **5** |
| `rc::rt::Pacer` | 10 ms | **0** |

The grid keeps its total and loses its spacing. A controller sees several updates in a
few milliseconds, each with a dt far below the period, then silence. A `dt` of 2 ms
under a derivative gain tuned for 10 is a spike five times too large.

## Fix

When a cycle overruns, drop the slots it lost and wait for the next point on the grid.
Never run them in a burst:

```cpp
const Nanoseconds lost = (behind + period_ - 1) / period_;   // whole slots overrun, rounded up
missed_ += lost;
deadline_ = next + lost * period_;                            // back on the grid, in the future
return deadline_ - now;
```

Count what was dropped, because a loop that loses cycles quietly is a loop nobody knows
is failing (`E-RT-0008`).

If catching up is genuinely wanted, as in a data logger that must not lose samples, that
is a different loop with a different contract, and the controller should not be it.
