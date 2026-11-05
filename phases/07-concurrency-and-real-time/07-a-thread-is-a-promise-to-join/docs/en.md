# A Thread Is a Promise to Join: What Happens to One That Is Not

> A function started a thread, then threw before the line that joined it. The
> program ended with status 134, and the catch block waiting for the exception never
> ran.

**Type:** Build
**Time:** about 105 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 07-01, 02-03, 02-08, 00-09

## The Problem

Every thread in phase 07 so far has been started and then joined, in a test, by
someone who remembered. Nothing has said what happens to one that is not.

It is worse than a leak. A robot that starts a worker thread, hits an error in the
next step, and unwinds is correct on the path where nothing goes wrong and ends the
whole process on the path where something does, at the moment it most needs to report
what happened.

## The Concept

### A started thread is a promise

A `std::thread` that has been started is **joinable**: somebody will either join it,
waiting for it to finish, or detach it, letting it run on its own. Break the promise
and the standard says `std::terminate`.

The first three end the process and the fourth does not, so each was measured as its
own program. gcc 11, with the exit status the shell saw:

| what the program did | what it printed | status |
| --- | --- | --- |
| destroyed a thread without joining | `terminate called without an active exception` | **134** |
| a thread's body threw `runtime_error("sensor lost")` | `terminate called after throwing an instance of 'std::runtime_error'`, `what(): sensor lost` | **134** |
| threw after starting a thread, before `join` | `terminate called without an active exception`, and the `catch` never ran | **134** |
| detached a thread that used a local | nothing; printed `1` where `42` belonged | **0** |

Status 134 is 128 plus `SIGABRT`, which lesson 00-09 explained, and like every crash
in this curriculum it arrives with no last line of output.

`E-THREAD-0008`.

### The third row is the one that catches careful programmers

```cpp
std::thread t(work);
step_that_can_throw();       // throws
t.join();                    // never reached
```

The join is on the next line and the function looks correct. A throw unwinds the
stack, `t` is destroyed while still joinable, and `std::terminate` runs **before any
catch block can**. The code is right on the path that does not throw, and the path that
does is the one that kills the process.

This is lesson 02-03's argument again. Cleanup that is a line of code is skipped by
every path that does not reach it. Cleanup that is a **destructor** is not.

### The second row: nobody can catch it

An exception that escapes a thread's function cannot be caught by anyone. There is no
caller: the thread's first function is the bottom of its stack. A `try` around
`std::thread t(f)` in the starting thread catches only what the starting throws, not
what `f` does, because `f` runs somewhere else.

So the catch has to be at the **top of the thread body**, and what it catches has to be
kept, because there is nobody to hand it to yet.

`E-THREAD-0009`.

### The fourth row: detach outlives its locals

```cpp
int local = 41;
std::thread t([&local] { sleep(50ms); local += 1; print(local); });
t.detach();
return;                      // local is gone
```

Measured on gcc 11. The plain build ran to completion, exited with status 0, and
printed 1. With the address sanitizer's default options it printed 200000001 and
**reported nothing**. With `ASAN_OPTIONS=detect_stack_use_after_return=1`, which lesson
02-10 explained, gcc and clang both reported `stack-use-after-return` at the line that
read the local. The thread sanitizer reported a data race.

So the tool that should have found it did not, until it was asked the right question.
Detach is almost never the answer.

`E-THREAD-0010`.

### A scoped thread

```cpp
~ScopedThread() { join(); }
```

That is the core of it, and it is worth more than it looks. Every way out of a scope,
a return, a break, a throw, ends in a join. The class is `std::thread` plus four rules:

- **One owner.** The copy operations are deleted. A copy would be two objects each
  promising to join the same thread.
- **Move is allowed, and move assignment joins first.** The defaulted assignment would
  destroy a joinable thread, and that is `std::terminate`.
- **Joining twice is harmless.** `join()` checks `joinable()` first.
- **The body is wrapped.** An exception that escapes is caught and kept, in state held
  on the heap, so that moving the `ScopedThread` does not move the memory the running
  thread is writing to.

```
  how three threads ended:
    threw runtime_error("sensor lost")   failed 1, "sensor lost"
    returned normally                    failed 0, ""
    threw the int 42                     failed 1, "unknown exception"
```

Asking `failed()` or `error()` **joins first**. The thread has to have finished for its
result to exist, and the join is the point where its writes become visible, which makes
reading them safe with no lock.

The suite shows each promise kept:

```
  start_then_throw: the exception reached the catch block yes, and the thread had finished yes
  eight threads started, container cleared: 8 of 8 had finished
```

The first is the function that would have ended the process with a bare `std::thread`.

## Build It

Six pieces of `ScopedThread`.

The constructor starts the thread with a lambda that captures a plain `State*` and a
copy of the body, and wraps the call in a `try` with two `catch` clauses, one for
`std::exception` and one for everything. The destructor calls `join()`. Move
assignment, if it is not self assignment, joins and then takes the other's thread and
state. `join()` checks `joinable()`. `failed()` and `error()` join and then read.

The stubs start nothing, so every promise in the suite is unkept in a way that cannot
hurt: the threads never ran, nothing is left joinable, and no test ends the process.

## Use It

```cpp
#include <rc/rt/scoped_thread.hpp>

rc::rt::ScopedThread worker([&] { read_sensor(); });
step_that_can_throw();
if (worker.failed()) log(worker.error());
```

`std::jthread`, from C++20, does the joining part of this. This baseline is C++17, and
the class is how you learn what `jthread` is for.

Two limits.

**It does not stop a thread.** A destructor that joins waits for the body to finish. A
body that loops until told to stop needs the stop flag from lesson 07-01, or a closed
queue from lesson 07-06, and the join waits for those too.

**It reports failure after the fact.** The body has already stopped when `failed()`
answers. Whether the rest of the robot can carry on without it is a decision for the
code that asks.

## What Breaks First

`E-THREAD-0008` is the abort with one line and no cause, because a thread was never
joined, including the version where the join was on the next line.

`E-THREAD-0009` is the process that died of an exception thrown in a thread nobody
could catch it in.

`E-THREAD-0010` is the detached thread reading a variable that no longer existed, which
exited 0 and printed the wrong number.

## Ship It

`rc/rt/scoped_thread.hpp`, in `namespace rc::rt`.

From here on, no lesson in this curriculum holds a bare `std::thread`, and none
detaches.
