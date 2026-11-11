# The Plan That Froze the Window: A Long Computation, the Event Loop It Stopped, and One That Can Be Cancelled

> Work in the thread that runs the interface stops the interface. Put it on another thread, report back by signal, and let it be cancelled.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 12-02, 09-11, 07-14

## The Problem

The operator presses Plan. The station works out a path for a second, and for that second the window does not repaint, the buttons do not click, the heartbeat monitor of the last lesson does not look and the link is not read. The robot is still moving, and the person at the screen cannot stop it.

## The Concept

### A clock that stopped

A timer that ticks every 10 ms, and a handler that works out a plan of 200 ms in the thread that runs the loop. The largest gap between ticks was **210** ms: the clock stopped for the whole plan. Everything else that depends on the loop stopped with it. See `E-QT-0102`.

### The same plan elsewhere

`PlanRunner` starts the work with `QtConcurrent::run` and watches its future with a `QFutureWatcher`, so that its progress, its result and its cancellation come back by signal on the thread that asked. The same plan, 20 steps of 10 ms, started in the runner: the largest gap was **11** ms, there was **1** result and it was **20**.

### Where it arrives

The result and every progress value arrived on the thread that asked, in order, each a step further than the last, and the last was **20**. A second `start` while one was running was refused and did not disturb the first; when the plan had finished a new one could start. See `E-QT-0103`.

### Cancelling

A plan of a second, cancelled after 50 ms: **1** cancellation, **0** finished, and it was over after **55** ms on this machine, well inside the limit the test sets of 500. It had got to step 1 of 100. The plan checks `promise.isCanceled()` at every step, which is what makes the cancel prompt: a cancel is a request, and work that does not look at it does not stop. See `E-QT-0104`.

### Destroying it

A runner destroyed 30 ms into a plan of a second cancelled the plan and waited for it before it went, and **0** results were delivered afterwards. A watcher whose job is still running when it is destroyed is a job that delivers into nothing.

## Build It

`PlanRunner::start`, `cancel` and `on_finished` in `exercise/solution.hpp`. The work, `plan_work`, is supplied.

```
rcpp verify 12-04
```

## Use It

`rc/qt/plan_runner.hpp` joins its module.

```cpp
rc::qt::PlanRunner runner;
QObject::connect(&runner, &rc::qt::PlanRunner::progress, this, [this](int p) { show_progress(p); });
QObject::connect(&runner, &rc::qt::PlanRunner::finished, this, [this](int r) { show_plan(r); });
runner.start(steps, 10);        // and runner.cancel() from the Stop button
```

**What this does not do.** It does one plan at a time, and it runs a function that takes a promise: a plan that reads or writes shared data needs the care of lesson 07-01, since another thread is now touching it. A cancel is a request, so a plan that does one long step cannot be stopped within that step, and the step has to be as short as the response you want. The thread comes from the global thread pool of Qt, which is as large as the machine has cores, and a station that starts many long jobs at once queues them. And the progress is a count of steps and says nothing of the time left.

## What Breaks First

- **A plan worked out in a handler.** See `E-QT-0102`.
- **A result delivered on the wrong thread.** See `E-QT-0103`.
- **A plan that cannot be stopped.** See `E-QT-0104`.

## Ship It

`rc/qt/plan_runner.hpp` holds `plan_work` and `PlanRunner`.

The capstone of this phase plans without stopping its link.
