# The Slot That Ran Inside Itself: processEvents in a Handler, Nested Calls and a Variable That Changed Under Its Owner

> A handler does some work and, to keep the program responsive, runs the event loop for a moment. The event loop
> delivers the next reading to the same handler, which has not finished with the first.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 09-03, 09-04

## The Problem

A handler takes a reading and does some work on it. The work takes a while, and the interface freezes, so somebody adds a call to `QCoreApplication::processEvents()` in the middle: "let the program breathe". It looks harmless, and it works on the bench, where readings arrive slowly.

Lesson 09-03 sent readings across a thread boundary and lesson 09-04 asked who deletes an object. This one asks what happens when a handler lets the loop run while it is in the middle of itself, which is neither of those, and is just as easy to do by accident.

## The Concept

### The handler is entered again

`processEvents` runs the event loop. The loop delivers whatever has arrived, and what has arrived is the next reading, for the same handler. The handler is **entered again**, from inside the call it is already in.

Reading 1, with readings 2 and 3 arriving while it works:

```
begin 1, begin 2, end 2, begin 3, end 3, end 1
```

The handler was nested to a depth of **2**. Not 3: Qt does not deliver an event again from a nested call while the batch it belongs to is still being delivered, so reading 3 waited for reading 2 to finish. See `E-QT-0036`.

### What it breaks

The handler keeps the reading it is working on in a member, runs the loop, and then checks that it is still working on it. Reading 1 came back to find the member holding **3**: **1** reading corrupted. Which one is hit depends on timing, so it shows under load and not on the bench. See `E-QT-0037`.

### A guard that queues

A flag that refuses the second call loses the reading. The repair is a line:

```cpp
pending_.push_back(value);
if (running_) return;
running_ = true;
while (!pending_.empty()) { next = pending_.front(); pending_.pop_front(); process(next); }
running_ = false;
```

A call made while one is in progress puts its reading in the line and returns. The call in progress works through the line when it has finished the one it is on.

With the guard, the log was **begin 1, end 1, begin 2, end 2, begin 3, end 3**, at a depth of **1**, with **0** overwritten. A single reading was handled exactly as before. 200 queued readings were each handled once and in order: the first entry `begin 1`, the last `end 200`, and 400 entries between them. See `E-QT-0038`.

## Build It

The guarded branch of `on_reading` in `exercise/solution.hpp`. The unguarded branch, `process`, and the counters are given.

```
rcpp verify 09-09
```

The test posts readings 2 and 3 from a hook that runs while reading 1 is in progress, which is what the world does to a real handler.

## Use It

`rc/qt/reentrancy.hpp` joins `rc::qt`. The class there is `ReadingHandler`.

```cpp
rc::qt::ReadingHandler handler(true);
connect(sensor, &Sensor::reading, &handler, &rc::qt::ReadingHandler::on_reading);
```

**What this does not do.** The guard makes the handler safe against being entered again and does not make `processEvents` a good idea: the line can grow without limit if readings arrive faster than they are handled, and each reading waits for all those before it. The right fix for a handler that is too slow is to
do its work elsewhere (lesson 09-03). It handles one thread: a handler used from two threads needs a lock as well. And it is Qt's rule about batches that kept the nesting to 2 and not 3, which has not been checked on every Qt version, so a test that asserts the exact log can differ on one.

## What Breaks First

- **`processEvents` in a handler.** The handler is entered again. See `E-QT-0036`.
- **A member used across it.** Somebody else's value. See `E-QT-0037`.
- **A repair that drops or reorders.** A line is what is wanted. See `E-QT-0038`.

## Ship It

`rc/qt/reentrancy.hpp` holds `ReadingHandler`.

Lesson 09-04 said the event loop has to run for `deleteLater` to act. This is the other side of that: when it runs matters, and running it from inside a handler is how a handler ends up inside itself.
