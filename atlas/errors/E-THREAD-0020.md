id: E-THREAD-0020
title: A std::async that was not parallel, because the future was thrown away
match: (ignoring return value of .*nodiscard|ignoring return value of function declared with 'nodiscard'|std::async.*discarded)
platforms: linux, windows
teaches: 07-13-getting-an-answer-back
---

## Symptom

Work started with `std::async` in a loop, meant to run in parallel, takes as long as running it one
after another. Four tasks that should overlap took four times as long. Every call returns only when
its task has finished.

The compiler said so, in a warning that is easy to read past:

```
warning: ignoring return value of 'std::future<...> std::async(...)',
         declared with attribute 'nodiscard' [-Wunused-result]            (gcc 11)
warning: ignoring return value of function declared with 'nodiscard'
         attribute [-Wunused-result]                                      (clang 14)
```

## Cause

`std::async` returns a future, and **the destructor of a future from `std::async` waits for the
task**. Discard the future and it is destroyed at the end of the statement, so the statement does
not finish until the task does. The call that was meant to start work in the background runs it in
the foreground.

Measured on gcc 11, four tasks that each wait up to 100 ms for the other three, so that overlapping
takes about 100 ms and running in turn about 400:

| | peak running at once | took |
|---|---|---|
| futures discarded | **1** | **400 ms** |
| futures kept | 4 | 100 ms |

Two things beside it. The default launch policy is the library's choice, and the standard allows
`deferred`, which runs the task on the thread that asks for the answer: on gcc 11 the default ran on
another thread, and `std::launch::deferred` ran on the calling one. And each call starts a thread:
2000 trivial calls took 52 ms, about 26 microseconds each.

## Fix

Keep the futures until all the work is started, then ask for the answers:

```cpp
std::vector<std::future<int>> futures;
for (int i = 0; i < 4; ++i) futures.push_back(std::async(std::launch::async, work, i));
for (std::future<int>& f : futures) results.push_back(f.get());
```

Say `std::launch::async` when parallel is the point. For many small tasks use a pool and not a thread
per call:

```cpp
#include <rc/rt/submit_future.hpp>
auto answer = rc::rt::submit_future(pool, [] { return compute(); });
```

If a future really is meant to be dropped, say so with `static_cast<void>(...)`, which silences the
warning and records that it was a decision. Do not turn the warning off.
