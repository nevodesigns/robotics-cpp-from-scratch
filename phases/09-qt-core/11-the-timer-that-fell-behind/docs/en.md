# The Timer That Fell Behind: QTimer Intervals, a Blocked Loop and the Ticks Nobody Counted

> A timer set to 10 ms is not a promise of a tick every 10 ms. It is a promise of no tick sooner.
> Re-arm it from its handler and it runs slow; block the loop and the ticks in the gap are gone.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 09-09, 07-08

## The Problem

Lesson 07-08 built a pacer that holds a rate on a thread of its own. In a Qt program the loop is usually the event loop and the clock is a `QTimer`, and the same three things go wrong, in new ways: the work shifts the timer, a blocked loop eats ticks, and anything that counts ticks as time is wrong.

## The Concept

### Re-armed from the handler

A single shot timer started with `start(10)` at the end of the handler is 10 ms from now, and now is after the work. In 300 ms with a handler that takes 3 ms, a re-armed timer ticked **22** times where 30 periods of 10 ms went by. See `E-QT-0042`.

### A blocked loop

A repeating timer does not queue what it could not deliver. One handler held the loop for 100 ms: of 40 slots in 400 ms there were **30** ticks and **10** missed. Qt did not deliver a burst afterwards. See `E-QT-0043`.

### Counting ticks as time

With the same block, a plain 10 ms timer counted **30 or 31** ticks over three runs, which is **300 or 310** ms, in **400** ms of real time: **90 to 100** ms short. See `E-QT-0044`.

### A grid

`slot_at` is the number of whole periods since the start. `delay_to_next_ms` is the time left in the current one, rounded up so that the timer is never early. `Ticker` aims a single shot timer at the next boundary after the handler has run, reads the slot from the clock when it fires, and says how many slots went by without a tick. In every run `ticks() + missed() == last_slot()`, and no slot is delivered twice.

## Build It

`slot_at`, `delay_to_next_ms` and `Ticker` in `exercise/solution.hpp`.

```
rcpp verify 09-11
```

## Use It

`rc/qt/ticker.hpp` joins `rc::qt`.

```cpp
Ticker ticker(10);
QObject::connect(&ticker, &Ticker::tick, [&](long long slot, long long missed) {
  if (missed > 0) { /* hold the last command, or stretch this step */ }
  step(slot);
});
ticker.start();
```

**What this does not do.** It does not make the loop real time: a tick still comes when the loop is free, and a block still costs its slots. It tells you so. It gives no bound on lateness inside a slot, and the numbers above are one machine's. What the suite asserts are the laws: the counts add up, slots only go forward, and a block loses ticks and not their count. A control loop that cannot tolerate a block belongs on a thread of its own (lessons 07-08 and 09-03).

## What Breaks First

- **A timer re-armed after the work.** See `E-QT-0042`.
- **Ticks lost to a block.** See `E-QT-0043`.
- **Ticks used as the time.** See `E-QT-0044`.

## Ship It

`rc/qt/ticker.hpp` holds `slot_at`, `delay_to_next_ms` and `Ticker`.

Lesson 07-08 held a rate on a thread by aiming at a deadline and not at an interval. This does the same on the event loop, where the loop can be taken away for a hundred milliseconds, and says how many ticks went with it.
