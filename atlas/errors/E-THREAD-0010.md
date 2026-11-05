id: E-THREAD-0010
title: A detached thread reading a variable that no longer exists
match: (stack-use-after-return|detached thread|thread outlived)
platforms: linux, windows
teaches: 07-07-a-thread-is-a-promise-to-join
---

## Symptom

A background task produces a wrong number, or a different wrong number on each run, or
works for weeks and then does not. Nothing crashes. A rerun in a debugger is fine.

## Cause

`detach` lets a thread outlive the function that started it, and it still holds
whatever that function gave it, including references to its locals:

```cpp
int local = 41;
std::thread t([&local] { sleep(50ms); local += 1; print(local); });
t.detach();
return;                      // local is gone
```

Measured on gcc 11: the plain build ran to completion, exited with status 0, and
printed **1** where **42** belonged. With the address sanitizer's default options it
printed 200000001 and **reported nothing**. With
`ASAN_OPTIONS=detect_stack_use_after_return=1`, which lesson 02-10 explained, gcc and
clang both reported `stack-use-after-return` at the line that read the local. The
thread sanitizer reported a data race.

So the tool that was supposed to find it did not, until it was asked.

## Fix

Do not detach. Join, with an object that cannot forget to:

```cpp
rc::rt::ScopedThread worker([&] { ... });     // joined before local goes away
```

If a thread genuinely must outlive its starter, give it what it needs **by value**, or
through a `std::shared_ptr` it shares ownership of, and let it signal when it is done.
A reference to a local is never that.

Run threaded code with `ASAN_OPTIONS=detect_stack_use_after_return=1` as well as under
the thread sanitizer, since the first is the one that names this bug.
