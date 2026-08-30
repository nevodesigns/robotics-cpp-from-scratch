# Threads: The Half Written Value Nobody Sees Coming

> A robot reads a sensor on one thread and steers on another. The steering thread will eventually see a position that the robot was never in.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 03-05

## The Problem

Every robot has at least two things happening at once. A sensor arrives when it
arrives, and the control loop runs on its own schedule, and neither can wait for
the other. So the sensor is read on one thread and the controller runs on
another, and they share the reading between them.

The obvious way to share is a variable. One thread writes it, the other reads it.

That works for a while, and then the controller sees a pose whose x came from one
reading and whose y came from the next. The robot was never at that position. It
is not a slightly stale value, which would be fine; it is a value that never
existed.

Worse, the standard does not call this a wrong answer. It calls it undefined
behaviour, which means the compiler was entitled to assume it could not happen
and to optimise on that assumption.

## The Concept

### A data race is undefined behaviour, not a wrong number

Two threads accessing the same memory, at least one of them writing, with no
synchronisation, is a **data race**. The consequence is not that you get one
value or the other. The consequence is that the program has no defined meaning.

That matters because the usual mental model, that a race just makes you read a
slightly old value, licenses reasoning that does not hold. A compiler that can
assume no races is free to keep a variable in a register across a loop, so a
thread may never see another thread's write at all, forever.

### A structure is written in pieces

A pose is three doubles. Writing one is three separate stores, and a reader can
arrive between any two of them.

```cpp
shared_pose.x = 1.0;   // reader could run here
shared_pose.y = 2.0;   // or here
shared_pose.theta = 0.5;
```

That is a **torn read**: a value assembled from two different writes. The
exercise detects it directly, by writing values with a known relationship and
checking that the relationship still holds on the way out.

Nothing about this is exotic. It is the ordinary consequence of a structure
being larger than one store.

### volatile is not the answer

This is worth stating plainly because it is the most common wrong fix in
robotics code, inherited from embedded C where it means something different.

`volatile` tells the compiler not to optimise away accesses, which is correct
and necessary for a hardware register that changes on its own. It says nothing
about atomicity and nothing about ordering between threads. A `volatile` pose is
still written in three stores and still tears.

For talking to a memory mapped device, `volatile`. For talking between threads,
never.

### A mutex makes a section indivisible

```cpp
{
  const std::lock_guard<std::mutex> held(mutex_);
  latest_ = reading;
}   // released here, on every path out
```

`std::lock_guard` is lesson 02-03 again: it acquires in its constructor and
releases in its destructor, so the lock is released on an early return and while
an exception unwinds. Locking and unlocking by hand is how a robot deadlocks.

What a mutex buys is that no other thread can be inside a section guarded by the
same mutex, so a half written structure is never observed. It also establishes
ordering, so everything the writer did before releasing is visible to whoever
acquires next. That second guarantee is the one people forget exists, and it is
why a mutex is enough on its own.

### What a mutex costs, and why phase 07 continues

Correct, and not free.

A thread that cannot take the lock waits, and how long it waits depends on what
the holder is doing. That is unbounded from the waiting thread's point of view,
which is exactly what a control loop with a deadline cannot accept. Worse, a
low priority thread holding a lock can block a high priority one, which is
called priority inversion and has ended missions.

So a mutex is right for a value published occasionally and read occasionally,
which is most sharing in a robot. It is wrong on the path that must finish in a
millisecond, and the next lesson builds what goes there instead.

### Testing cannot find races, and here is the measurement

This lesson ships four tests that ought to catch the unsynchronised version.
Running it five times on this machine:

```text
a reading is never seen half written        failed 5 times out of 5
many writers and many readers never tear    failed 5 times out of 5
every publication is counted under load     failed 0 times out of 5
a worker thread notices the stop flag       failed 0 times out of 5
```

The last two describe real races. A lost update and a flag cached in a register
are both undefined behaviour and both genuinely present in that code. The tests
simply did not observe them, on this compiler, on this machine, today. A
different optimisation level or a busier machine would give a different table.

That is the property that makes concurrency bugs so expensive: **a race that
does not reproduce is still a race**, and it will reproduce on the robot, at
temperature, in front of a customer.

The tool that does not depend on luck is the thread sanitizer. It instruments
every memory access and reports a race the first time one becomes possible,
whether or not it went wrong. On the same unsynchronised code it reports five
races immediately, naming the exact line, and on the corrected version it
reports none.

Run it on threaded code. Not sometimes.

### An atomic flag for stopping

Not everything needs a mutex. A single value that fits in a machine word can be
read and written atomically:

```cpp
std::atomic<bool> running{true};
```

That is exactly right for telling a thread to stop, and it needs no lock. It is
not right for a pose, because three doubles do not fit in a word.

## Build It

Implement in `exercise/solution.hpp`:

- `LatestReading::publish(reading)` and `LatestReading::latest()`, sharing one
  reading between a producer thread and a consumer thread without tearing.
- `LatestReading::count()`, how many have been published.
- `StopFlag`, an atomic flag with `request_stop()` and `stopped()`.

```
rcpp verify 07-01
```

The tests run real threads. One writes readings whose fields have a fixed
relationship and the other checks that relationship on every read, several
hundred thousand times, which is what makes a torn read show up rather than
remain theoretical.

## Use It

`std::shared_mutex` allows many simultaneous readers and one writer, which suits
a value read far more often than written. `std::scoped_lock` takes several
mutexes at once without the deadlock that taking them one at a time invites.

Run threaded code under the thread sanitizer, which finds races that testing
alone will not:

```
cmake --preset tsan && cmake --build build/tsan
setarch $(uname -m) -R ctest --test-dir build/tsan
```

The `setarch -R` is not decoration. On Linux kernels from about 6.6 onward, the
thread sanitizer refuses to start with:

```text
FATAL: ThreadSanitizer: unexpected memory mapping
```

because the kernel places mappings where it does not expect. Disabling address
space randomisation for that process is the standard workaround, and without it
the tool that finds your races does not run at all. See `E-TSAN-0001`.

## What Breaks First

- **A value that was never written by anybody.** A torn read, from sharing a
  structure without synchronisation. See `E-THREAD-0001`.
- **A thread that never notices the flag and runs forever.** A plain bool shared
  between threads, which the compiler may keep in a register. See
  `E-THREAD-0002`.
- **A lock that is never released.** Locking by hand rather than with a guard,
  and an early return in between. See `E-CPP-0017`.

## Ship It

`LatestReading` joins `rc::rt` and is how every sensor thread in this curriculum
publishes. The next lesson measures what its lock costs and builds the queue that
goes on the deadline path.
