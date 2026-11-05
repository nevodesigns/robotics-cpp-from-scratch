id: E-THREAD-0008
title: terminate called without an active exception, because a thread was never joined
match: (terminate called without an active exception|destroyed a joinable thread|joinable thread was destroyed)
platforms: linux, windows
teaches: 07-07-a-thread-is-a-promise-to-join
---

## Symptom

The program aborts with one line and nothing else:

```
terminate called without an active exception
Aborted (core dumped)
```

Status 134. No exception was thrown, no message names a file, and the output that was
buffered is gone.

## Cause

A `std::thread` that is started is a promise: somebody will join it or detach it
before it goes away. Destroy one that is still **joinable** and the standard says
`std::terminate`.

Measured on gcc 11, as its own program, because it ends the process: a function that
started a thread and returned. Status 134, which is 128 plus `SIGABRT`; lesson 00-09
explains that arithmetic.

The version that catches careful programmers has the join on the next line:

```cpp
std::thread t(work);
step_that_can_throw();       // throws
t.join();                    // never reached
```

Measured: status 134, and the `catch` block waiting for the exception **never ran**.
The throw unwinds the stack, `t` is destroyed while joinable, and `terminate` runs
before any handler can. Code that is correct on the path that does not throw is not
correct.

## Fix

Attach the join to the object and not to the code path, which is lesson 02-03:

```cpp
#include <rc/rt/scoped_thread.hpp>

rc::rt::ScopedThread worker(work);
step_that_can_throw();       // throws: the destructor joins on the way out
```

Every way out of the scope ends in a join: a return, a break, a throw. Do not hold a
bare `std::thread` in this curriculum. `std::jthread` does the same job from C++20, and
this baseline is C++17, so the class is the point.
