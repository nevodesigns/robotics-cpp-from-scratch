# How Many Workers: A Thread Pool, and Why the Answer Is Not the Number of Cores

> Twelve workers on a twelve thread machine made a job seven times faster, and
> forty eight made it seven times faster. The same forty eight, doing tasks that
> wait, made it forty four times faster.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 07-06, 07-07, 02-08

## The Problem

Lessons 07-06 and 07-07 built the two parts of a worker pool without calling it one: a
queue a thread can sleep on, and a thread that is always joined. This puts them
together. The interesting part is the question the pool has to answer that neither part
could: **how many workers?**

Most people answer with the number the machine reports, and that is right for exactly one
kind of task.

## The Concept

### Computation scales to the cores, and stops

Experiment, method stated. An i7-10850H: 6 cores with 2 hardware threads each, so
`std::thread::hardware_concurrency()` says 12. 48 tasks of about 20 ms of real computation
each, 960 ms of work in all, by the number of workers. A fixed number of loop iterations,
calibrated, run twice:

```
  workers     1      2      4      8     12     24     48
  speedup   1.0x   2.0x   3.5x   6.1x   6.6x   6.8x   7.2x     (run 1)
  speedup   1.0x   2.0x   3.8x   6.4x   7.7x   7.4x   7.4x     (run 2)
```

It scales with the cores and then stops. Twelve workers did not give twelve times: the two
threads of a core share execution units, so a second hardware thread is worth roughly
twenty percent, not a core. More workers than hardware threads gave nothing, and cost memory
and switching.

The first version of this experiment was wrong in a way that flattered. It spun until a
**wall clock** deadline, so an oversubscribed task finished on time without ever having been
given the CPU, and reported speedups of 15 and 20 on 12 hardware threads, which is
impossible. Computation has to be a fixed amount of work and not a duration. A speedup
larger than the core count was the tell.

`E-THREAD-0014`.

### Waiting scales to what is waited on

The same 48 tasks if they each **wait** 20 ms, as a task blocked reading a device does:

```
  workers     1      4     12     48
  speedup   1.0x   4.0x  11.9x  44.6x
```

A waiting task uses no core, so it scales far past the core count. The right number of
workers depends on what the tasks do. For computation, about the number of physical cores.
For waiting, as many as there are things to wait on, within reason. A pool sized once and
used for both is wrong for one of them.

### What a pool is for, besides the count

4000 trivial tasks. A thread started and joined for each, 122 ms. The same on a pool of 4,
**5.4 ms**.

A thread costs about thirty microseconds to start and stop. That is nothing to a task that
runs for a second and most of the time of one that runs for a microsecond. A pool pays the
cost once.

### The design

- **Tasks are `std::function<void()>`, the queue is lesson 07-06's, the workers are lesson
  07-07's.** A worker is always joined, and an exception has somewhere to go.
- **A task that throws is counted, and the worker carries on.** Lesson 07-07 measured that
  an exception escaping a thread ends the process. Worse is a worker that catches it at the
  top of its thread and exits: one bad task a day retires one worker a day, and the pool
  slowly stops with no error. It is caught per task, at the one place that runs tasks.

  ```
    10 tasks, three of which throw: ran 10, completed 10, failed 3
  ```

  Five more submitted afterwards all ran. `failed()` is not decoration: a pool that swallows
  exceptions without counting them hides bugs.

  `E-THREAD-0016`.
- **Shutdown is: stop accepting, close the queue, join.** The queue hands out what it holds
  before it reports closed, so everything accepted runs.

  ```
    200 slow-ish tasks, then shutdown: accepted 200, ran 200, a late submit was refused
  ```

- **`wait_idle_for` answers "is everything I submitted finished?", with a deadline,** as
  every wait in this curriculum does.

### The race the first draft had

`submit` counts a task and pushes it. In the first draft those were two steps with the
lock released between them:

```cpp
{ lock; if (!accepting_) return false; ++outstanding_; }    // lock released here
queue_.push(task);                                          // shutdown can close the queue before this
```

If `shutdown` closes the queue in that gap, the workers drain what is there and leave, and
the task is pushed into a queue nobody reads, counted as outstanding for ever. The window is
a few instructions and the suite passed with the bug in. It was found by reading `submit`
against `shutdown`, not by running anything.

The fix is to make the decision and the push one step under the same lock `shutdown` takes to
stop accepting. Then shutdown comes either before, and the submit is refused, or after, and
the task is already queued and is drained. There is no third case.

`E-THREAD-0015`.

## Build It

Three methods; the constructor, the worker loop, `shutdown` and the counters are given.

`submit` locks, refuses if the pool is no longer accepting, and otherwise counts the task
**and pushes it, inside the lock**. `wait_idle_for` waits on the idle condition with a
predicate and the limit. `run_task` runs the task in a `try`, catches everything, and then
under the lock counts it as completed, counts a throw as failed, and notifies when nothing is
outstanding.

`shutdown` is given on purpose. A stub that did not close the queue would leave the workers
running, and the pool's destructor would wait for them for ever, so the exercise would hang
instead of failing.

The stub refuses every task, which cannot strand one. The suite fails it in ten
milliseconds.

## Use It

```cpp
#include <rc/rt/worker_pool.hpp>

rc::rt::WorkerPool pool(4);
for (const Scan& scan : scans) pool.submit([&scan] { process(scan); });
if (!pool.wait_idle_for(std::chrono::seconds(5))) { /* something is stuck */ }
```

Size it for the work. Four to six for computation on this machine, a few dozen for tasks that
mostly wait, and two pools if one program does both.

On the thread sanitizer, expect a lot of noise from **GCC 11**: this suite reported 69 problems
there, 61 data races, 6 double locks, and two `lock-order-inversion (potential deadlock)`. It is
the false positive from lesson 07-06, `E-TSAN-0002`: GCC 11's runtime does not see the lock handed
back inside a timed condition variable wait, so its record of held locks goes stale, and it even
invents a cycle. The code takes the two locks in one order only, pool then queue, in `submit`.
Clang 14's reported nothing in 15 runs. Read the code for a reversed order before believing a
cycle report, and use clang's.

Two limits.

**It runs tasks, it does not order them.** Tasks are taken in the order submitted, but run
concurrently, so two tasks that touch the same data still need a lock, or to be one task.

**A task that never returns holds a worker for ever.** There is no cancellation, and
`wait_idle_for` reporting false is the only sign. Give tasks their own deadlines.

## What Breaks First

`E-THREAD-0014` is the pool sized for the machine, with a speedup that stopped at seven.

`E-THREAD-0015` is the task submitted to a pool that had shut down, counted as outstanding for
ever.

`E-THREAD-0016` is the one task that threw and the pool that slowly stopped working.

## Ship It

`rc/rt/worker_pool.hpp`, in `namespace rc::rt`.

From here on, work that can run beside the control loop in this curriculum goes to a pool sized
for what it does, and every task's failure is counted.
