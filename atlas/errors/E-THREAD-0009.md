id: E-THREAD-0009
title: terminate called after throwing, from a thread nobody could catch it in
match: (terminate called after throwing an instance of|exception escaped a thread|uncaught exception in a thread)
platforms: linux, windows
teaches: 07-07-a-thread-is-a-promise-to-join
---

## Symptom

A robot process dies at an arbitrary moment with the message of an exception, and the
`try` block around the code that started the thread had nothing to do with it:

```
terminate called after throwing an instance of 'std::runtime_error'
  what():  sensor lost
Aborted (core dumped)
```

## Cause

An exception that escapes a thread's function cannot be caught by anyone. There is no
caller: the thread's first function is the bottom of its stack. The runtime calls
`std::terminate`, and the whole process goes.

A `try` around `std::thread t(f)` in the starting thread does not help, because `f`
runs in another thread. It catches only exceptions thrown by starting it.

Measured on gcc 11, as its own program: a thread whose body threw
`std::runtime_error("sensor lost")` and was then joined. Status 134, with the
message above.

## Fix

Catch at the top of every thread body, and keep what was caught so that someone can ask:

```cpp
try { body(); }
catch (const std::exception& e) { state->error = e.what(); state->failed = true; }
catch (...) { state->error = "unknown exception"; state->failed = true; }
```

`rc::rt::ScopedThread` does this and reports it as a value the caller has to look at:

```cpp
rc::rt::ScopedThread worker([] { read_sensor(); });
if (worker.failed()) log(worker.error());
```

Measured: a body that threw `runtime_error("sensor lost")` gave `failed` true and
`"sensor lost"`. One that returned normally gave false and an empty string. One that
threw the int `42` gave `"unknown exception"`, because `catch (...)` has no message to
give. Asking waits for the thread, which is also what makes reading the result safe.

A thread that fails has to be able to say so. A robot that loses its sensor thread and
carries on regardless is worse than one that stops.
