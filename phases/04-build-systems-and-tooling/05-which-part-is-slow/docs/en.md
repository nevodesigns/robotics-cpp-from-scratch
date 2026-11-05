# Which Part Is Slow: Attributing Time Inside One Loop

> The four named parts came to 880 microseconds of a 1000 microsecond loop. The
> missing 120 went somewhere nobody had thought to name, which is the usual
> place for a surprise.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 02-03, 03-05, 03-08

## The Problem

A control loop misses its deadline.

Phase 07 will measure *that* it missed, and by how much, with a latency
histogram. This lesson answers the next question, which is **which part** of it
was slow, and you have to answer that before changing anything.

On a desktop you would attach a profiler. On a robot you frequently cannot: the
interesting failure happens once an hour, on a machine you reach over a
network, inside a loop that stops behaving the moment anything attaches to it.

So the loop measures itself.

## The Concept

### Three things you have already built

This lesson is mostly assembly, which is the point of having a library you
wrote.

**The RAII guard from 02-03**, so a section cannot be left open by an early
return. **The injectable clock from 03-05**, so every number in this lesson's
tests is exact rather than statistical. **The iterator from 03-08**, so the
report sorts like any other container.

The clock is worth dwelling on. Every other timing lesson in this curriculum
has had to fight flakiness: 05-05 found the machine drifting mid run, and 07-05
needed best of N on a loaded builder. Here the clock is a `TestClock` the test
advances by hand, so the suite asserts `250 * kMicrosecond` exactly and will do
so on every machine forever.

### A loop, broken down

```
section                  calls      total us       share
read sensors                 1         250.0       25.0%
estimate                     1         400.0       40.0%
control                      1          80.0        8.0%
write motors                 1         150.0       15.0%
```

Four sections, 880 microseconds, in a loop that took 1000.

**The 120 microseconds that are missing is the most useful number in the
report.** It is the copying between the stages, the logging, the bounds check,
the thing somebody added last month. It is unnamed because nobody thought it
was worth a section, which is exactly why it is where the unexpected thing
lives.

Forty percent in `estimate` is visible and probably already known. Twelve
percent that nobody can account for is not.

So `unaccounted_for` takes the whole loop's duration and subtracts what the
sections explained. One subtraction, and it is the only number in the report
that tells you about code you did not instrument.

### And when it goes negative

Nest a section inside another and the outer one measures wall time, so it
includes the inner:

```
outer 700.0 us, inner 500.0 us, loop 700.0 us
accounted for 1200.0 us, which is more than the loop took
unaccounted_for reports -500.0 us
```

Exactly the nested section, counted twice.

It would be easy to clamp that at zero and never show a negative. Do not. **A
negative remainder is the only signal you get that the shares add up to more
than a hundred percent.** Clamping turns a detectable mistake into a report
that is merely wrong.

Keep sections flat: one level, covering disjoint stretches. Nest them
deliberately when you mean to, and then do not add the shares up.

### Why a guard and not a begin and end

```cpp
timer.begin("read sensors");
if (!sensor.read(&value)) return false;    // the end never runs
timer.end("read sensors");
```

Every way out of a scope that is not the bottom of it breaks this: an early
return, a `break`, a `continue`, a throw. And error paths are where early
returns live, so the instrument is most wrong precisely when something is going
wrong, which is when you are reading it.

With a guard, the destructor runs on every path. Measured: a function that
gives up halfway recorded 40 microseconds and one call, and the next section
started clean instead of inheriting an open one's start time.

The guard deletes its copy and move operations, because a section is a place in
the code and two guards for one section would record it twice. It is still
returned by value from `measure()`, which works because C++17 guarantees the
elision for a prvalue. That is lesson 01-11's measurement being relied on
deliberately rather than hoped for.

### The mean is not the number you want

Five iterations of one section:

| | microseconds |
|---|---|
| mean | 282 |
| worst | **900** |

Four passes near 110 and one at 900. A one millisecond deadline is not at risk
according to the mean and was very nearly missed in fact.

Lateness is not an average. This is the same shape 05-05 measured directly: a
distribution with a floor and no ceiling, where the median tells you almost
nothing about the tail, and the tail is the only part a deadline cares about.
So the timer keeps the worst alongside the total, which is one comparison per
measurement.

### What the instrument costs

Measured with a clock that counts how many times it was asked: **exactly two
reads per section.**

So a 500 Hz loop with ten sections asks the clock 10000 times a second. That is
arithmetic rather than opinion, and whether it matters is a question about your
clock rather than about this code.

The timer also has fixed capacity and allocates nothing, because a measurement
that allocates changes the thing being measured.

### One bug this lesson's own tests caught

Worth recording, because it is the same mistake in a different place.

The first `reset()` set the section count back to zero and left the slot data
alone. It looks correct. It is not: the next section registered lands in slot
zero, finds the previous run's calls and total still sitting there, and adds to
them.

That is stale state surviving a reset, which is precisely the failure a section
left open produces. Moving a counter is not the same as clearing the data
behind it.

## Build It

Implement the `Scope` constructor and destructor, `accounted_for`,
`unaccounted_for`, `index_of` and `record` in `exercise/solution.hpp`.

```
rcpp verify 04-05
```

The suite breaks a loop into four sections and finds the part nobody named,
nests two and watches the remainder go negative, accumulates five iterations
and keeps the worst, calls a function that gives up halfway, counts the clock
reads the instrument costs, and sorts the report.

## Use It

**Measure the whole iteration as well as the parts**, and print the
difference. The gap is the point.

**Keep sections flat, and never clamp the remainder.**

**Open a section with a guard, never a pair of calls.** If `begin` and `end`
are not exposed at all, the unbalanced version cannot be written.

**Compare the worst against the budget, not the mean.**

**Reach for `rc::rt::Histogram` from 07-03** once you need percentiles rather
than a maximum.

**Clear the data when you reset**, not just the count.

## What Breaks First

- **Sections that do not add up to the loop.** See `E-DEBUG-0003`.
- **A section left open by an early return.** See `E-DEBUG-0004`.
- **A mean that hides the iteration which missed.** See `E-DEBUG-0005`.

## Ship It

`Section` and `SectionTimer` join `rc::core`. Phase 07 measures whether the
loop met its deadline and this says which part of it to look at, so the two are
usually switched on together.
