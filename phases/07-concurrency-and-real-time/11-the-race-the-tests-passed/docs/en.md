# The Race the Tests Passed: Forcing an Interleaving Instead of Hoping for One

> A race with a window of a few instructions was stranded a task in about 0.07
> percent of trials. A suite that ran it 100 times passed with the bug in about 93
> percent of the time.

**Type:** Build
**Time:** about 105 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 07-10, 07-09, 02-08

## The Problem

Lesson 07-10's worker pool had a race in its first draft, and the tests passed.

`submit` counted a task and pushed it as two steps, with the lock released in between.
`shutdown` could close the queue in that gap, and the task would be pushed into a queue
nobody would read again. The window is a few instructions wide. It was found by reading,
not by running anything.

That is a problem for a curriculum whose whole method is to prove claims by running them.
This lesson is how to test a race you suspect, and how to test the ones you do not.

## The Concept

### Running it a lot finds it, barely

Experiment, method stated: the pattern reduced to its essentials, two real threads, one
submitting four tasks and one calling shutdown, 20000 trials at each of three delays,
three runs. How often the buggy version stranded a task:

| setting | run 1 | run 2 | run 3 |
| --- | --- | --- | --- |
| shutdown at once | 11 | 2 | 9 |
| after 50 spins | 28 | 8 | 18 |
| after 200 spins | 22 | 7 | 14 |

in 20000 trials each. The fixed version stranded a task in **0 of 180000**.

So running it a lot does find it: about **0.07 percent** of trials, and as few as 2 in 20000
in a bad run. A suite that runs the race 100 times sees nothing about 93 percent of the time
with the bug still in. The count changes with the delay, the run and what the machine is doing
at the time, which is why a pass proves nothing and a failure cannot be reproduced.

Lessons 07-01 and 07-09 measured the same of a torn read and a deadlock. It is the same
finding a third time, for the most ordinary kind of race there is.

`E-TEST-0016`.

### The other approach: make the threads line up

Stop hoping, and name the places in the code where an interleaving could matter. Let a test
run an action at one of them.

```cpp
race_point("submit.gap");                                        // in the code under test
const RaceHook hook("submit.gap", [&] { intake.shutdown(); });   // in the test
```

With a hook at the gap that calls `shutdown()`, shutdown runs exactly where the bug needs it,
on **one thread**, inside the call, between the two lines the bug lives between. No second
thread, no timing.

```
  buggy intake, shutdown() run at submit.gap on the same thread:
    the hook fired        yes
    submit returned       true, the task was accepted
    tasks stranded        1
```

Across 300 trials with the interleaving forced, the buggy version stranded a task **300 times
in 300**, where running two real threads 300 times usually finds none. The failure is the same
every time, so it can be debugged.

### Then try every point

You will not always know the gap. A submit with three points can have shutdown run before it,
in the middle, or after, and those are all the interleavings of the two steps. Try each:

```
                   buggy intake                 fixed intake
    point          hook fired   stranded       hook fired   stranded
    submit.before      yes           0               yes          0
    submit.gap         yes           1                no          0
    submit.after       yes           0               yes          0
```

The buggy version strands a task at the gap and nowhere else. Before the call, the submit is
refused. After the push, shutdown finds the task queued and drains it. The fixed version has no
gap to hook, so that row never fires, and that is the repair. This is a small exhaustive search
and not a hope.

A task that is merely waiting, because shutdown has not run yet, is not stranded: `stranded()`
counts only what is still queued **after** shutdown has already run. The first version of this
measurement counted waiting tasks too and reported a bug in the fixed version, which was a bug in
the measurement.

`E-THREAD-0017`.

### The one rule

```cpp
void submit() {
  const std::lock_guard<std::mutex> held(mutex_);
  race_point("submit.inside");                    // the hook runs here, with mutex_ held
}
```

If the hook calls `shutdown()`, which locks `mutex_`, one thread waits for itself. Measured on a
small program: a point outside the lock returned at once, and a point inside it was killed by a
3 second timeout.

So **place race points where no lock is held**, and have hooks call the public operations, which
take their own locks. `race_point` copies the action out and runs it after releasing its own
table's lock, for the same reason.

`E-TEST-0017`.

### What it costs when nothing is installed

```cpp
if (hooks_installed().load(std::memory_order_acquire) == 0) return;
```

One atomic load, finds zero, returns. The table's mutex is never taken in the common case, so a
race point can stay in production code, and the suite checks that with nothing installed it does
nothing, and does nothing again once a hook is removed.

## Build It

Three pieces.

`race_point(name)` returns at once if no hooks are installed. Otherwise it locks the table, finds
the first entry with that name, copies its action out, leaves the lock, and **then** calls it.

`RaceHook`'s constructor locks the table, takes the next id, adds an entry, and counts one more
installed hook. Its destructor finds its entry **by id**, not by name, so two hooks at one point do
not remove each other, erases it, and counts one fewer.

The stubs do nothing, so no hook ever runs and no interleaving is forced. The suite checks that a
hook runs while installed and stops when removed, because "nothing installed does nothing" is true
of a stub.

## Use It

```cpp
#include <rc/test/race.hpp>

rc::test::race_point("pool.after_count");          // in the code, between the two steps

const rc::test::RaceHook hook("pool.after_count", [&] { pool.shutdown(); });
```

Add a race point wherever a check is followed by an act, a count by a push, or a read by a write,
and then try each one.

Run it under the thread sanitizer as well. The hooks are reached from several threads at once in
the suite, and nothing was reported under gcc 11's or clang 14's in 15 runs each.

Two limits.

**It finds the interleavings you named.** It is the partner of the stress run, which finds the ones
you did not, and of lesson 07-09's level check, which finds orders. None replaces the others.

**Hooks run on whatever thread reached the point.** A hook that assumes it is on the test's thread
will be surprised by one that reaches it from a worker.

## What Breaks First

`E-TEST-0016` is the concurrency test that passed with the bug still in, with the table of how
rarely a plain run finds it.

`E-THREAD-0017` is the task stranded between the check and the act.

`E-TEST-0017` is the hook that made the test wait for itself, because the point was inside a lock.

## Ship It

`rc/test/race.hpp`, in `namespace rc::test`.

From here on, a class in this curriculum with a check followed by an act has a race point between
them, and a test that tries each.
