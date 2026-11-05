id: E-THREAD-0024
title: A stop callback running while the thing it captured was destroyed
match: (callback was still running|stop callback outlived|use after free in a stop callback|destroyed while the callback was running)
platforms: linux, windows
teaches: 07-14-asking-work-to-stop
---

## Symptom

An occasional crash, or a corrupted value, at shutdown, in code that registers a callback on a stop token with a lambda
that captured locals by reference. The stack, when there is one, is in the lambda. Under the address sanitizer:
`stack-use-after-return`, or a use of freed memory.

## Cause

The thread that requests the stop runs the callbacks. The thread that owns the callback object may destroy it, and
the locals the lambda captured, at the same moment. If the destructor does not wait, the lambda runs against memory
that is gone.

```cpp
std::mutex mutex; std::condition_variable woken; bool signalled = false;
StopCallback wake(token, [&] { lock(mutex); signalled = true; woken.notify_all(); });
// ... returns, destroying wake, then signalled, woken, mutex ...
```

Measured, forced: a callback held open on the requesting thread, and its owner destroyed on another. A destructor that
returned at once would have freed the captures with the callback still using them. The destructor waited, and the
callback finished first.

## Fix

The destructor must wait for a callback that is in progress:

1. Remove the entry from the list, under the list's lock.
2. Then take a second lock that the requester holds for the whole run, and let it go.

And the requester must take that second lock **before** it copies the list, so there is no moment when a callback is
about to run and its owner has already gone. If the copy came first, a destructor could erase its entry, find nothing
running, and return while the requester held a copy that still included it.

Declare the callback **after** the things it captures, so it is destroyed before them.

This costs something. A callback must not destroy its own `StopCallback`, since that would wait for itself, and a callback
that blocks holds up every destructor behind it. `std::stop_callback` makes the same trade.
