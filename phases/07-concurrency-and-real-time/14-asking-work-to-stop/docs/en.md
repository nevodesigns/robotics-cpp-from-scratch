# Asking Work to Stop: A Flag Cannot Wake a Sleeper, and a Callback Must Not Outlive What It Captured

> A worker polling a stop flag took a median of 3.3 ms to stop. One woken by the
> request took 72 microseconds.

**Type:** Build
**Time:** about 115 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 07-06, 07-07, 07-13, 02-08

## The Problem

A flag tells a thread to stop. Lesson 07-01 built one. It has two limits: it cannot wake a thread
that is asleep, and it says nothing to anything waiting on something else. Lesson 07-13 ended on the
third: a task in the pool cannot be told to stop at all.

C++20 has `std::stop_token`, `std::stop_source` and `std::stop_callback` for this. This curriculum is
C++17, so they are built by hand, which is how to learn what they are for.

## The Concept

### How fast it stops depends on how it waits

Measured on gcc 11, 40 trials each, how long after the request a worker had stopped:

```
  a flag polled between 10 ms sleeps      median 3.27 ms   worst 9.3 ms
  a wait the request itself wakes         median 72 us     worst 105 to 122 us
```

Polling is not wrong, it is slow, in proportion to its sleep. The expected delay is half the interval
and the worst is all of it. A wait the request wakes is about 45 times quicker at the median and 80
at the worst, because nothing has to be noticed: the request does the waking. Shortening the poll
only trades latency for wakeups, which is `E-THREAD-0006`.

`E-THREAD-0022`.

### A thread that is computing stops as fast as it looks

A loop that checks the flag every N iterations, 15 trials each:

```
  checked every 1 iteration         median 0.9 us     worst 2.5 us
  checked every 1000 iterations     median 2.1 us     worst 3.7 us
  checked every 100000 iterations   median 80 to 240 us   worst 313 to 355 us
```

How often a loop checks is how fast it can be stopped. The suite shows the same thing in counted
iterations, which are exact:

```
  the request is made at iteration 100:

    checks every        stopped at iteration
               1                     100
              64                     128
            1000                    1000
          100000                  100000
```

And a task that never checks cannot be stopped at all. That is what **cooperative** means, and why
nothing here can cancel a task that does not look.

### Three pieces

- **`StopSource`**, the end that asks. `request_stop()` returns true for the one call that made the
  request, and false for any after it.
- **`StopToken`**, the end that looks. A cheap copy of a handle, safe to give to any number of threads.
- **`StopCallback`**, a function to run when the request comes. It runs **at once** if the request has
  already been made, so registering late cannot miss it. A callback destroyed before the request does not
  run.

With a callback, stopping the consumers of a queue is one line: a callback that calls `queue.close()`.

```
  three consumers asleep with a 1500 ms wait, then request_stop(): 3 of 3 returned closed, within 0 ms
```

A flag could not do that. A flag is something to look at, and a sleeping thread is not looking.

`E-THREAD-0023`.

### The careful part

The thread that requests the stop runs the callbacks. The thread that owns a callback may destroy it, and
the locals its lambda captured, at the same moment. If the destructor does not wait, the lambda runs against
memory that is gone.

Forced in the suite: a callback held open on the requesting thread, and its owner destroyed on another.

```
    destroyed while the callback was still running   no, it waited
    the callback finished before the destructor did  yes
```

The rule has two halves. The destructor removes its entry and then **waits for a request that is running**.
And the requester takes that lock **before** it copies the list of callbacks. If the copy came first, a
destructor could erase its entry, find nothing running, and return, while the requester held a copy that still
included it and then ran it against memory that was gone.

This costs something. A callback must not destroy its own `StopCallback`, since that would wait for itself,
and a callback that blocks holds up every destructor behind it. `std::stop_callback` makes the same trade.

`E-THREAD-0024`.

### A sleep the request ends

```cpp
rc::rt::sleep_unless_stopped(token, std::chrono::milliseconds(500));
```

A local mutex and condition variable, and a callback that sets a flag under the mutex and notifies. The callback
is declared **after** the things it captures, so it is destroyed before them. Measured in the suite: a 5000 ms
sleep with a stop requested after 50 ms woke after 50 ms.

## Build It

Four pieces.

`request_stop` does the exchange so only the first call goes on, takes the run mutex, **then** copies the list
under the list's mutex, and calls each action outside that lock.

`StopCallback`'s constructor checks `stopped` under the list's mutex, and either pushes an entry or runs the action
at once, after the lock is released. Its destructor erases the entry and then locks and releases the run mutex.

`sleep_unless_stopped` returns at once if already requested, and otherwise waits on a condition that a callback
signals.

The stubs ask nobody and register nothing, so the suite fails them in a few seconds and nothing hangs.

## Use It

```cpp
#include <rc/rt/stop.hpp>

rc::rt::StopSource source;
rc::rt::StopCallback closes(source.get_token(), [&] { queue.close(); });
rc::rt::ScopedThread worker([token = source.get_token(), &queue] {
  while (!token.stop_requested()) { /* take an item with a deadline and work on it */ }
});
source.request_stop();      // wakes the consumer, and the worker's loop sees the token
```

Give a long computation a token and check it at the granularity you can afford to be stopped at.

Run it under the thread sanitizer: clang 14's reported nothing in 15 runs. GCC 11's reports the timed wait falsely
(`E-TSAN-0002`).

Two limits.

**It is cooperative.** A task that does not look cannot be stopped, and `std::terminate` is not a stop.

**It does not order shutdown.** Stopping the producer before the consumer, so nothing is dropped, is a design this
only makes possible.

## What Breaks First

`E-THREAD-0022` is the worker that took milliseconds to stop because it polled a flag between sleeps.

`E-THREAD-0023` is the stop that a thread asleep on a queue never heard.

`E-THREAD-0024` is the callback that ran while the thing it captured was destroyed.

## Ship It

`rc/rt/stop.hpp`, in `namespace rc::rt`.

From here on, long running work in this curriculum takes a `StopToken`, waits in a way a request can end, and
checks at a granularity stated beside it.
