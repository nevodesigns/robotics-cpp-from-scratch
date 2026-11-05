# Priority Is a Share, and Inversion Is a Bug: Why the Highest Priority Task Waited for the Middle One

> A process at nice 19 against one at nice 0 got 1.6 percent of the core. A task
> of the highest priority waited 15 ticks for one that outranked nothing.

**Type:** Build
**Time:** about 115 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 07-09, 07-08

## The Problem

A robot has tasks of different importance: the control loop, the estimator, the logger, the map builder.
The obvious thing is to give them priorities, and the obvious reading of a priority is "who goes first".
Two things about priorities are not what they sound like. They are **shares**, and they can **invert**.

## The Concept

### A priority is a share

Experiment, method stated. Linux, two CPU-bound processes pinned to one core with `taskset`, 3 seconds
each, millions of loop iterations done:

| pair | first | second | the second's share |
| --- | --- | --- | --- |
| nice 0 against 0 | 748 | 751 | 50 percent |
| nice 0 against 10 | 1330 | 145 | **9.8 percent** |
| nice 0 against 19 | 1448 | 24 | **1.6 percent** |

A lower priority is not "runs a little less". At nice 19 the process got a sixtieth of the core.

The kernel gives each nice value a weight, 1024 at nice 0, 110 at nice 10, 15 at nice 19, and each
process gets its weight over the sum of the weights competing:

```
                         predicted      measured
    nice 0 against 10    90.3 percent   90.2 percent
    nice 0 against 19    98.6 percent   98.4 percent
```

The arithmetic predicted the measurement, which is why the suite can test the arithmetic. That
is with competition for one core. On a machine with idle cores the low priority process runs
happily and the problem never shows, until the robot is busy.

`E-RT-0011`.

### And an ordinary program cannot ask for more

Measured, unprivileged:

```
  sched_setscheduler(SCHED_FIFO, 10)    refused: Operation not permitted     (priorities run 1 to 99)
  nice(-5), a higher priority           refused: Operation not permitted
```

Real time priorities need root, `CAP_SYS_NICE` or a raised `RLIMIT_RTPRIO`, set up where the robot is
installed. A program that depends on one must check the result and make the fallback visible, because
the call fails quietly if nobody reads it.

`E-RT-0010`.

That is also why what follows is a simulation. The scheduler's choices cannot be forced or repeated
portably, and a lesson cannot assume a privilege. A simulation makes every tick exact, and runs
identically on all four toolchains.

### Priority inversion

Three tasks and one lock. **Low** (1) takes the lock. **High** (3) wakes and needs it, so it waits for
Low. **Medium** (2) wakes. It does not need the lock, it outranks Low, so it runs, and it keeps running
while it has work. Low cannot finish the work that releases the lock for as long as Medium runs.

High is waiting for Medium, which it outranks, by way of Low, which it was never meant to wait for.

```
    ticks     0123456789012345678
    who ran   LLHMMMMMMMMMMLLLHL

    High finished at tick 16, 15 ticks after it woke. Medium finished at 12, before it.
```

A task that outranks nothing in the picture finished before the highest. That is the signature. This is
the bug that repeatedly reset the Mars Pathfinder lander in 1997, and the fix that was uploaded to it was
turning priority inheritance on.

`E-RT-0009`.

### Inheritance

A task holding a lock runs at the highest priority of any task waiting for it.

```
    who ran   LLHLLLHMMMMMMMMMML

    High finished at tick 6, 5 ticks after it woke, against 15.
    Medium finished at 16, against 12: it pays 4 ticks for it.
```

Low, holding the lock High needs, runs as if it were High, finishes, releases, and High goes. Medium
pays four ticks, which is the cost and the point.

### It has to travel the whole chain

High waits for a lock Medium holds, and Medium is itself waiting for a lock Low holds. A busy task N,
priority 3, sits between Medium (2) and High (4):

```
                           trace                    High done   N done
    no inheritance         LLNNNNNNNNNNLLMH               15        11
    through the chain      LLLLMHNNNNNNNNNN                5        15
```

Inheriting only one step gives Low Medium's priority, 2, and N, at 3, still beats it, so the inversion
survives. Low has to inherit High's 4 **through** Medium. An answer that looked one step passed the
classic scenario and failed exactly this one, which is why the second scenario exists.

## Build It

Two functions; the simulator around them is given.

`cpu_share(nice_a, nice_b)` is `weight_of(a) / (weight_of(a) + weight_of(b))`, as doubles. `weight_of` holds
the kernel's table and clamps nice to the range -20 to 19.

`effective_priority(tasks, holder, index, inheritance, depth)` starts from the task's own priority. With
inheritance off, or beyond depth 8, that is the answer. Otherwise, for every task that is not done and is
blocked on a lock that `index` holds, it takes the larger of its own and that waiter's **effective**
priority, recursively. The depth limit means a deadlock cannot recurse for ever.

The stubs say half, and the task's own priority, which is no inheritance. The baseline scenario passes
against them, because that is the bug, and every other test fails.

## Use It

```cpp
#include <rc/rt/priority_sim.hpp>

const rc::rt::Result result = rc::rt::simulate(tasks, 1, true);
// result.trace is who ran each tick; result.finished is when each finished
```

Write your own tasks' scripts as lock, compute and unlock steps and see what your design does before it
meets a real scheduler.

On a real system, use a mutex that has the protocol, `pthread_mutexattr_setprotocol` with
`PTHREAD_PRIO_INHERIT`. `std::mutex` does not promise it. Or avoid sharing a lock across priorities at all,
which is what lesson 07-12's seqlock and lesson 07-02's queue are for.

Two limits.

**It is a model.** One core, fixed priorities, locks that take no time, no time slicing between equals. It
shows the mechanism, and the real scheduler has more in it.

**Inheritance fixes inversion, not the lock.** A lock held for a long time is still held for a long time.
The cure for that is holding it for less, as lesson 07-09 said.

## What Breaks First

`E-RT-0009` is the highest priority task that waited for the medium one, by way of the low one.

`E-RT-0010` is the real time priority that was refused, and the program that carried on without it.

`E-RT-0011` is the low priority process that got almost nothing, because priority is a share.

## Ship It

`rc/rt/priority_sim.hpp`, in `namespace rc::rt`.

From here on, a design in this curriculum that shares a lock between priorities says how inversion is
prevented, and a task given a low priority says what share it will actually get.
