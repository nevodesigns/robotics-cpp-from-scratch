# Getting an Answer Back: Futures, the std::async That Was Not Parallel, and a Failure the Caller Has to Handle

> Four tasks started with std::async, in a loop, to run in parallel, ran one after
> another. The compiler had warned.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 07-10, 03-07, 02-08

## The Problem

Lesson 07-10's pool runs work and says nothing about it. A task returns `void`, so the
answer to "what did it compute?" has nowhere to go, and a task that threw is counted and gone.

The standard library has a machine for carrying an answer back from another thread, `std::future`,
and it has three traps in it that this lesson measures.

## The Concept

### std::async looks parallel, and a discarded future makes it serial

Four tasks that each wait up to 100 ms for the other three, so that running together takes about
100 ms and one after another about 400. gcc 11:

```
                        peak running at once   took
    futures discarded         1               400 ms
    futures kept              4               100 ms
    submit_future, kept       4               100 ms
```

`std::async` returns a future, and **the destructor of a future from `std::async` waits for the
task**. Discard it and it is destroyed at the end of the statement, so the statement does not finish
until the task does. The call meant to start work in the background runs it in the foreground.

Both gcc 11 and clang 14 warn about it, as `-Wunused-result` from the `nodiscard` attribute, and the
warning is the right one. The suite discards the futures on purpose, with `static_cast<void>`, which
says it was a decision and keeps the build warning free.

`E-THREAD-0020`.

Two more measurements beside it. The **default launch policy is the library's choice**: on gcc 11 it
ran on another thread, and the standard allows `deferred`, which runs the task on the thread that asks
for the answer. `std::launch::deferred` ran on the calling thread. And **each call starts a thread**:
2000 trivial calls took 52 ms, about 26 microseconds each, which is lesson 07-10's reason for a pool.

### A pool wants a packaged_task, and it is move-only

`std::packaged_task` wraps a callable and hands out a future. The pool's tasks are `std::function`,
which must be copyable. Measured:

```
error: static assertion failed: std::function target must be copy-constructible     (gcc 11)
error: call to deleted constructor of 'std::packaged_task<int ()>'                  (clang 14)
```

So the task lives in a `std::shared_ptr`, and the lambda handed to the pool copies the pointer:

```cpp
auto task = std::make_shared<std::packaged_task<Result()>>(std::move(fn));
std::future<Result> result = task->get_future();
pool.submit([task] { (*task)(); });
```

One task, shared by the pool's copy of the lambda and nobody else.

`E-CPP-0073`.

### What comes back

A value, of any movable type: the suite returned an `int`, a `std::string` and a
`std::unique_ptr<int>`, and nothing.

**An exception.** A task that threw `runtime_error("sensor lost")`:

```
    get() threw           "sensor lost"
    pool completed        2
    pool failed           0   (the future caught it and kept it for the caller)
```

`get()` rethrows the same exception in the caller. And the pool saw nothing wrong: `failed()` stayed at
0, because `packaged_task` caught the exception and kept it for the future. There are two ways of
reporting a failure, and they are separate. A plain task is counted by the pool; a task behind a future
is delivered to whoever holds the future.

**A refusal.** A task submitted after the pool shut down is destroyed unrun, and `get()` throws
`std::future_error`:

```
  a task submitted after shutdown: get() threw future_error "std::future_error: Broken promise"
```

A refusal becomes an exception and not a hang.

`E-THREAD-0021`.

### Never get() without a limit

`get()` waits for ever on a task that never finishes. `wait_for` never blocks longer than its argument:

```cpp
bool ready_within(std::future<R>& future, std::chrono::milliseconds limit) {
  return future.wait_for(limit) == std::future_status::ready;
}
```

Measured: it gave up at 40 ms on a task held on a gate, and answered yes once the gate opened. Every wait
in this curriculum has a deadline for the same reason.

## Build It

Two function templates.

`submit_future(pool, fn)` works out the result type with `decltype(fn())`, makes a `shared_ptr` to a
`packaged_task` of it, takes its future, hands the pool a lambda that copies the pointer and calls the
task, and returns the future. It catches nothing: `packaged_task` already stores an exception for the
future, and a refused submission destroys the task unrun, which the future reports.

`ready_within(future, limit)` is `wait_for(limit) == ready`.

The stubs return a future that already holds an error and say no, so a caller fails with a message and
never waits for ever. The suite runs the same four tasks through `submit_future`, `std::async` kept and
`std::async` discarded, so the learner's function appears in the comparison and not beside it.

## Use It

```cpp
#include <rc/rt/submit_future.hpp>

rc::rt::WorkerPool pool(4);
std::future<Map> map = rc::rt::submit_future(pool, [&] { return build_map(scan); });
// ... other work ...
if (rc::rt::ready_within(map, std::chrono::milliseconds(200))) use(map.get());
```

Run threaded code under the thread sanitizer, and use clang's: the pool waits with a timeout, which
gcc 11's reports falsely (`E-TSAN-0002`). This suite reported nothing under clang 14's in 12 runs.

Two limits.

**A future is one shot.** `get()` can be called once. For a result several threads read, publish it
somewhere else, such as a seqlock.

**No cancellation.** A task in the pool cannot be told to stop, and `ready_within` returning false only
means it has not finished. Lesson 07-14 is how a task is asked to stop.

## What Breaks First

`E-THREAD-0020` is the `std::async` that was not parallel, because the future was thrown away.

`E-CPP-0073` is `std::function` refusing a task that can only be moved.

`E-THREAD-0021` is the `get()` that threw, either the task's exception or a broken promise.

## Ship It

`rc/rt/submit_future.hpp`, in `namespace rc::rt`.

From here on, a task in this curriculum that has an answer returns it through a future with a limit on
the wait, and none uses `std::async` to mean "in the background".
