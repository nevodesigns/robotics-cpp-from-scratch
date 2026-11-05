# A Loop That Holds Its Rate: Sleeping Until a Deadline, and What to Do When a Cycle Overruns

> A loop meant to run at 100 Hz ran at 82.6, and the line that did it looked
> exactly right.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 07-03, 03-05

## The Problem

Lesson 07-03 measured how late a loop is. It did not say how to write one that is
not. The loop everybody writes first is

```cpp
while (running) { work(); std::this_thread::sleep_for(period); }
```

and it does not hold a rate. A controller written for 100 Hz and run at 83 integrates,
differentiates and filters over the wrong `dt`, and nothing in the program says so.

## The Concept

### Each cycle takes the work plus the period

```
  100 cycles, 2 ms of work, a 10 ms period, wanting 100 Hz, simulated:

                              100th cycle started at    each interval    rate
    sleep a period after                 1188 ms          12 ms       83.3 Hz
    your Pacer                            990 ms          10 ms      100.0 Hz
```

The arithmetic is exact because the time is simulated. On real threads and a real clock,
gcc 11, the same 100 cycles run three times, to within a few milliseconds each time:

| how the loop waited | total | rate |
| --- | --- | --- |
| `sleep_for(period)` after the work | 1210 ms | **82.6 Hz** |
| `sleep_until` a fixed grid | 1000 ms | 99.99 Hz |

A 20 percent error from a line that looks right.

`E-TIME-0003`.

### Sleeping for what is left still drifts

The obvious repair measures the work and sleeps for the rest of the period. 300 cycles,
real clock:

| | total | late per cycle |
| --- | --- | --- |
| `sleep_for(period - work)` | 3032 ms | **105 to 110 us** |
| `sleep_until` a fixed grid | 3000 ms | about 1 us |

1.1 percent slow. Every sleep overshoots a little, as every sleep does, and nothing
ever corrects it, so the overshoots are **added up**.

A grid never asks how long a sleep took. It asks where the next deadline is, and that
number is computed from the start and not from the last result. The error of one cycle
is gone by the next instead of being kept.

### The grid has a failure of its own

A plain fixed deadline, after a stall, is behind, so there is nothing to sleep for, and
it runs the missed cycles one after another until it has caught up.

Experiment, method stated: one 45 ms cycle in a 10 ms loop, real threads, a fixed
deadline. It started 5 cycles late, back to back, and the total time came out at 300.06
ms against an ideal 300. Simulated, 20 cycles with the same stall:

```
                          shortest interval    intervals shorter than the period
    a plain fixed grid               2 ms                       5
    your Pacer                      10 ms                       0
```

The grid keeps its **total** and loses its **spacing**. A controller sees several
updates a couple of milliseconds apart, each with a `dt` far below the period, then
silence. A derivative term tuned for 10 ms sees 2 and spikes five times too large, and a
rate counter reports nothing wrong because the average is right.

`E-TIME-0004`.

### So drop what was lost, and count it

When a cycle overruns, the slots it overran are **dropped**, the loop waits for the next
point on the grid, and it never bursts.

```
  work of 8, 14, 35, 2, 2 ms in a 10 ms period, when each cycle started (ms):

    sleep a period after       0   18   42   87   99
    your Pacer                 0   10   30   70   80

    slots dropped: 4
```

Every start is on the grid, however long the overrun. The 14 ms cycle dropped one slot
and the 35 ms cycle dropped three.

The count is not decoration. A loop that loses cycles quietly is a loop nobody knows is
failing, and 4 dropped slots out of 5 is a loop running at about two thirds of its rate,
reported as fine.

It costs more than people expect. Late by **one nanosecond** drops the whole slot: the
cycle that finishes at 40 ms and 1 ns waits until 50. That is the price of never
bursting, and it is written down so that it is a decision.

`E-RT-0008`.

### The time is an argument

```cpp
rc::core::Nanoseconds sleep_needed(rc::core::Nanoseconds now);
```

The pacer never reads the clock. That is lesson 03-05's idea: a loop that reads the time
itself cannot be tested without waiting, and one that is handed it can be tested in no
time at all. Every test in this lesson is arithmetic on simulated nanoseconds, so a
hundred cycles, a 45 ms stall and a clock that went backwards run instantly and give exact
answers, where timing real sleeps would give numbers that depend on the machine that day.

## Build It

One method, `sleep_needed(now)`.

Work out `next = deadline_ + period_`, from the grid and never from `now`. If `now` is on
time, the wait is `next - now`, and the grid moves to `next`. If the wait would be more
than a whole period, the clock went backwards or the call came early, so wait one period
and start the grid again from here.

If `now` is late, count the whole slots overrun, rounding up, add them to `missed_`, move
the grid to the first point at or after `now`, and return the wait to it.

The stub returns 0 forever, the loop that never pauses. The suite names it by the rate it
gives.

## Use It

```cpp
#include <rc/rt/pacer.hpp>

rc::rt::Pacer pacer(period_ns, now_ns());
while (running) {
  work();
  std::this_thread::sleep_for(std::chrono::nanoseconds(pacer.sleep_needed(now_ns())));
  if (pacer.missed() > 0) report(pacer.missed());
}
```

`std::this_thread::sleep_until` with a deadline computed from the start is the same
idea, and the pacer is what adds the overrun policy. Use `std::chrono::steady_clock` for
the loop's own timing, which never goes backwards, so the guard in `sleep_needed` is a
second line of defence and not the first.

Two limits.

**It cannot make a sleep accurate.** The operating system wakes a sleeping thread when it
is ready to, not when asked, and lesson 07-03's histogram is how you see by how much. The
pacer stops the lateness being added up. It does not remove it.

**Dropping is a policy.** A data logger that must not lose a sample wants the opposite,
and should be a different loop with a different contract, not this one.

## What Breaks First

`E-TIME-0003` is the loop that runs at 83 Hz because it sleeps after the work.

`E-TIME-0004` is the loop that caught up after a stall with a burst of tiny steps.

`E-RT-0008` is the loop that lost cycles and said nothing, or waited for a clock that
had moved.

## Ship It

`rc/rt/pacer.hpp`, in `namespace rc::rt`.

From here on, a periodic loop in this curriculum sleeps until a deadline computed from
its start, drops and counts the slots an overrun lost, and is handed the time.
