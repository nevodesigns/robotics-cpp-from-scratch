# The Signal Sent Ten Thousand Times: A Fast Sensor, a Queued Connection and a Latest Value That Counts What It Replaced

> A queued signal is one event per emit, and Qt does not merge them. A sensor faster than the interface
> fills a queue, and the interface works through values it never needed to show.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 09-03, 08-09

## The Problem

Lesson 09-03 sent a value across the thread boundary safely. This sends ten thousand a second, which is how a sensor behaves, to an interface that wants the latest and nothing else.

## The Concept

### The flood

10000 emits of a queued signal were **0** calls before the loop ran and **10000** after it. None merged. See `E-QT-0048`.

### One event while one is waiting

`LatestValue::publish` stores the value and counts the write. If a delivery is not already waiting it posts one with `invokeMethod` and a queued connection, so it runs on the thread the object lives on. 10000 writes before the loop were **1** update, with the value 9999 and **9999** skipped. Three writes, two and one, each followed by the loop, were 3 updates that skipped **2**, **1** and **0**: the pending flag is cleared by the delivery. See `E-QT-0049`.

### From another thread

200000 writes from a second thread while the loop ran gave a few thousand updates, and the number changed from run to run, so the suite does not assert it. It asserts the law: **updates plus the sum of skipped equal the writes**, and the last value written is the last value delivered.

### Which thread

A connection with no receiver object is direct: the slot ran on **the sensor's thread**. The latest value's update, from a write on that same thread, ran on **the interface's**. See `E-QT-0050`.

## Build It

`publish` and `deliver` in `exercise/solution.hpp`.

```
rcpp verify 09-13
```

## Use It

`rc/qt/latest_value.hpp` joins `rc::qt`.

```cpp
rc::qt::LatestValue latest;                        // lives on the interface's thread
connect(&latest, &rc::qt::LatestValue::updated, this, [&](double v, long long skipped) { show(v); });
// from the sensor thread, at any rate:
latest.publish(reading);
```

**What this does not do.** It keeps one `double`, and a reading with a timestamp or several fields needs a struct and a different type. It is for what is shown, not what is logged: a log wants every reading, and lesson 08-09 is about that choice. It does not rate limit the interface: one delivery is waiting at most, and the interface can still be given one per loop pass. And its object must live on the thread whose loop is to run the update.

## What Breaks First

- **A queue that grows.** See `E-QT-0048`.
- **A loss nobody counted.** See `E-QT-0049`.
- **A slot on the wrong thread.** See `E-QT-0050`.

## Ship It

`rc/qt/latest_value.hpp` holds `LatestValue`.

Lesson 08-09 chose between the newest reading and all of them in plain C++. This is the newest, for the interface, with the count of what it stood for, and it is the piece that the capstone of this phase puts between a sensor thread and a screen.
