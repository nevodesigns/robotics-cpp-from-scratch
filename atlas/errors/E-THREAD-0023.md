id: E-THREAD-0023
title: A stop was requested and a thread asleep on a queue never heard it
match: (never heard the stop|asleep on a queue|a flag cannot wake|stop requested but the thread did not exit)
platforms: linux, windows
teaches: 07-14-asking-work-to-stop
---

## Symptom

`request_stop()` is called and one thread does not exit. It is asleep waiting for an item from a queue, or on
a condition variable, and it will stay there until something arrives or its timeout passes. The program cannot
exit while it is.

## Cause

A flag is a thing to be looked at, and a sleeping thread is not looking. Setting `stop = true` does nothing to a
thread inside `pop_for` or `wait`.

```cpp
stop.request_stop();     // sets a flag
worker.join();           // the worker is asleep in queue.pop(), and never reads it
```

Nothing can reach it except the thing it is waiting on. Lesson 07-06's `close()` wakes every waiter, but
something has to call it.

## Fix

A **callback** that runs when stop is requested, and does the waking:

```cpp
rc::rt::StopSource source;
rc::rt::StopCallback closes(source.get_token(), [&] { queue.close(); });
...
source.request_stop();          // closes the queue, which wakes every consumer
```

Measured: three consumers asleep on a queue with a 1500 ms wait, then `request_stop()`: **3 of 3 returned closed
within 0 ms**.

A callback registered after the request has already been made runs at once, where it is constructed, so
registering late cannot miss it. Measured: a late callback ran immediately, and a callback destroyed before the
request did not run at all.

C++20's `std::stop_callback` is this, and so is the whole idea of a cancellation token. A flag alone is the version
that works only on threads that are looking.
