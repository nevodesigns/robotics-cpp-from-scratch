id: E-THREAD-0005
title: A wakeup that never came, because the notification was sent before the wait
match: (notification was lost|lost wakeup|waits for ever for a notification)
platforms: linux, windows
teaches: 07-06-waiting-without-spinning
---

## Symptom

A consumer thread sleeps for ever, or for exactly as long as its timeout, although
the producer plainly did its work and notified. It happens rarely, usually at
start-up, and almost never on a developer's machine, which makes it the kind of bug
that ships.

## Cause

A condition variable does not remember. A notification wakes whoever is waiting at
that instant and is then gone.

```cpp
signal.notify_one();                         // nobody is waiting yet
signal.wait_for(lock, milliseconds(50));     // waits for a notification that already happened
```

Measured: notify first, then wait 50 ms with no condition checked, **timed out**.
The same wait, asking whether a flag was set first, **returned at once**.

So the thing being waited for is not the notification. It is a change of **state**,
kept under the mutex, and the notification only says "look again".

## Fix

Keep the condition as state, change it under the lock, and wait in a loop that
checks it:

```cpp
// producer
{ const std::lock_guard<std::mutex> held(mutex_); items_.push_back(item); }
ready_.notify_one();

// consumer
std::unique_lock<std::mutex> lock(mutex_);
while (items_.empty() && !closed_) ready_.wait_until(lock, deadline);
```

An item pushed before anyone asked is found by looking, so no notification is needed
and nobody sleeps. `rc::rt::BlockingQueue` is built this way, and its suite pushes
first and checks the consumer never went to sleep.

The same loop answers the other half of the problem. A wakeup does not mean the
condition is true: another consumer may have taken the item, and the standard allows
a wait to return when nobody notified at all.
