# Time: Where dt Comes From, and Which Clock to Ask

> Your PID controller has been taking dt as a number somebody handed it. Today you find out who, and what happens when they are wrong.

**Type:** Build
**Time:** about 105 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 03-04

## The Problem

Every controller, filter and watchdog in this curriculum takes `dt`, the seconds
since the last update. It has always arrived as a `double` from outside the
lesson, and that was a loan against this one.

Get it from the wrong clock and a robot in Europe lurches twice a year. Not
metaphorically: an operator changes the system time, or the machine syncs with a
time server and steps backwards by two hundred milliseconds, and every controller
computing `now - last` receives a negative interval. The derivative term
explodes, the integral unwinds the wrong way, and the machine does something
sudden.

That is not a rare bug. It is a scheduled one, and it has a specific cause.

## The Concept

### Two clocks, and only one of them measures duration

The standard library offers several clocks. Two matter.

`std::chrono::system_clock` is the wall clock: what a person would call the time.
It can be set by an administrator, stepped by a time synchronisation daemon, and
moved by daylight saving. **It can go backwards.** Its only job is to answer what
time is it, for a log line or a timestamp somebody will read.

`std::chrono::steady_clock` never goes backwards and never jumps. It counts
forward from an arbitrary starting point that means nothing on its own, and the
only useful thing you can do with it is subtract two readings. That is exactly
what measuring an interval is.

The rule is short and absolute:

**Measure elapsed time with `steady_clock`. Record when something happened with
`system_clock`. Never the other way round.**

A robot uses both, for different jobs, in the same program.

### Durations carry their units in the type

The genuinely good idea in `<chrono>` is that a duration is not a number. It
knows its own units, and the compiler converts between them or refuses.

```cpp
const auto timeout = std::chrono::milliseconds(500);
const auto tick = std::chrono::microseconds(2500);
const auto total = timeout + tick;     // fine, becomes microseconds
```

A function taking `std::chrono::milliseconds` cannot be handed seconds by
mistake. Compare that with a `double` named `timeout`, where the units live in a
comment or in somebody's memory, and where the mistake is silent. Confusing
milliseconds with seconds is a thousandfold error, and it has destroyed real
hardware.

Conversions that lose precision are refused unless you ask for them explicitly
with `duration_cast`, which is the compiler making you acknowledge the rounding
rather than discovering it later.

### Getting to a double, once, at the edge

Control mathematics wants seconds as a `double`. That conversion is fine, and it
belongs in exactly one place: the boundary where a duration becomes an input to
the maths.

```cpp
const double dt = std::chrono::duration<double>(now - last).count();
```

Doing it once, at the edge, keeps the type safety everywhere else. Doing it early
throws the units away at the top of the program and hopes.

### The clock has to be injectable, or nothing can be tested

A controller that calls `steady_clock::now()` inside itself cannot be tested. To
check what a watchdog does after two seconds you would have to wait two seconds,
and to check what happens at a leap you would have to arrange one.

So time comes in through an **interface**: a class that promises what can be
asked of it and says nothing about how the answer is found.

```cpp
class Clock {
 public:
  virtual ~Clock() = default;
  virtual Nanoseconds now() const = 0;
};
```

This is the first place in the curriculum that needs `virtual`, so it is worth
saying what the two lines do.

`virtual Nanoseconds now() const = 0` declares a function that every kind of
clock must provide and that this class does not implement. The `= 0` is what
makes it a promise rather than a default: a `Clock` cannot be created, only a
real clock or a test clock can. Calling `now()` through a `Clock*` then reaches
whichever one is actually there, decided while the program is running rather
than while it is compiling.

`virtual ~Clock() = default` is not decoration. Deleting a derived object
through a pointer to the base, with no virtual destructor, does not run the
derived destructor: whatever it owned is leaked and whatever it was going to
release is not released. The compiler will not mention it. Lesson 02-03 built
the destructor that has to run; this is the line that makes sure it does.

The real clock asks `steady_clock`. The test one returns whatever the test says,
and can be advanced by an hour instantly. Every timing test in this curriculum
from here on runs in microseconds of real time and covers behaviour spanning
hours, and lesson 05-02 uses the same shape for a sensor.

### What to do when dt is nonsense

Even with a steady clock, a `dt` can arrive wrong: a thread was descheduled, a
frame was dropped, the loop stalled. A controller cannot simply trust it.

Three cases worth handling explicitly, and the exercise implements all three.

A `dt` of zero or negative means no time passed or the clock moved backwards.
Nothing should be integrated or differentiated, so the sensible answer is to
report it and let the caller skip the update, which is what lesson 14-01's
controller already does.

A `dt` far larger than expected means the loop stalled. Integrating over it as
though it were real produces a large sudden correction, which on hardware is a
lurch. Clamping to a maximum is the usual answer, and it must be a deliberate
decision rather than an accident.

## Build It

`exercise/solution.hpp` gives you a `Clock` interface, a `SteadyClock` that reads
the real one, and a `TestClock` you can move. Implement:

- `TestClock::advance(ns)` and `TestClock::set(ns)`.
- `seconds_between(from, to)`, the interval as a `double`, the one conversion at
  the edge.
- `LoopTimer::tick(clock)`, returning the interval since the previous tick as a
  `TickResult`: the seconds, whether this was the first tick, and whether the
  value had to be corrected.
- The first tick reports `first` and a `dt` of zero, because there is no previous
  reading to subtract.
- A backwards or zero interval reports `Backwards` and a `dt` of zero.
- An interval above `max_dt` reports `Stalled` and a `dt` clamped to `max_dt`.

```
rcpp verify 03-05
```

Every test uses the test clock, so the whole suite covers hours of behaviour and
runs instantly.

## Use It

ROS 2 has exactly this distinction and adds a third case: simulated time, where
a clock is published by the simulator so a replay can run faster or slower than
real time. Code written against an injected clock gets that for free, which is
another reason the seam is worth having.

On Linux, `steady_clock` is `CLOCK_MONOTONIC`, which does not count time the
machine spends suspended. Where that matters, `CLOCK_BOOTTIME` does, and reaching
it needs a platform call rather than the standard library.

## What Breaks First

- **A negative dt, and a controller that lurches.** Elapsed time was measured
  with the wall clock, and something stepped it backwards. See `E-TIME-0001`.
- **A timeout a thousand times too long or too short.** Seconds and milliseconds
  were mixed through an untyped number. See `E-TIME-0002`.
- **A duration count that overflowed.** Nanoseconds in a 32 bit type run out in
  about four seconds. See `E-NUM-0002`.

## Ship It

`Clock`, `SteadyClock`, `TestClock` and `LoopTimer` join `rc::core`, and from
here every lesson that mentions `dt` gets it from this rather than from a
parameter with no history. Phase 07 measures loop jitter with it, and phase 14
feeds the controller you already wrote.
