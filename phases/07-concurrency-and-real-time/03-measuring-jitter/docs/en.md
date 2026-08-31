# Jitter: Measuring the Loop You Are About to Trust

> A control loop that runs at a thousand hertz on average and stalls for four milliseconds once a minute is not a thousand hertz loop. It is a loop with a bug you have not found yet.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 07-02

## The Problem

The last lesson measured something uncomfortable. The worst case of a queue
operation was not set by the queue at all; it was set by the operating system
deciding to run something else. Removing the lock did not remove it.

That leaves an obvious question, and it is the one every real robot has to
answer before anybody trusts it: **how late does this loop actually run, and how
often?**

Not on average. The average is reassuring and nearly useless. A loop that is
early or on time ninety nine times out of a hundred and four milliseconds late on
the hundredth has an average that looks perfect and a controller that is
periodically integrating over four times the interval it thinks it is.

The instrument for that question is a histogram, and this lesson builds it.

## The Concept

### Jitter is lateness, not variation

Say the loop is meant to run every thousand microseconds. It actually ran 1043
microseconds after the last time. It is 43 microseconds **late**, and that number
is what matters, because a controller that assumed 1000 has now integrated over
4 percent more time than it accounted for.

Two related quantities are worth keeping apart:

- **Interval**, how long since the previous run.
- **Lateness**, the interval minus the period it was supposed to be.

Lateness can be negative, meaning the loop ran early, which is usually harmless
and occasionally a sign the clock is not what you think.

### The mean is the wrong summary, and here is why

Timing distributions are not symmetric. A loop cannot run much earlier than its
period, because it waits, but it can run arbitrarily later, because something
else took the processor. The distribution has a hard floor and a long tail.

A mean over that tells you almost nothing about the tail, and the tail is the
entire question. Report percentiles instead:

- The **median** says what a typical cycle looks like.
- The **99th percentile** says what a bad cycle looks like, and there are
  thirty six of those an hour at a kilohertz.
- The **worst** says whether the system can miss a deadline at all.

A specification written as an average is not a specification. One written as
"the 99.9th percentile is under 200 microseconds and nothing exceeds 500" is.

### A histogram, and why not just keep every sample

Keeping every measurement means allocating without bound in the loop you are
measuring, which changes the thing you are measuring. Lesson 03-03 made the
point about allocation in a control loop, and it applies with particular force to
the code doing the timing.

A histogram is fixed memory decided up front: an array of counters, one per
range of values. Recording is an index calculation and an increment, with no
allocation, no branching worth the name, and no growth however long it runs.

Two design decisions matter.

**Bucket width** sets the resolution. Too wide and everything lands in one
bucket; too narrow and the tail is spread thinly across hundreds of empty ones.
For a kilohertz loop, ten microsecond buckets are usually about right.

**An overflow bucket** counts everything beyond the last range. Without one, a
four millisecond stall either lands in the final bucket, understating how bad it
was, or is lost. Counting overflows separately, and keeping the true worst value,
is what stops a histogram lying about its own tail.

### What causes the tail

Worth knowing before measuring, because the shape tells you which one you have.

**Scheduling.** Another process is runnable, and the kernel gives it the
processor. On an ordinary kernel your thread has no special claim.

**Page faults.** Memory that has not been touched yet, or has been swapped, costs
a trip to the kernel on first access. This is why real time processes call
`mlockall` at startup.

**Frequency scaling and power states.** A processor waking from an idle state
runs slower for a while.

**Your own code.** An allocation, a lock, a log line that writes to disk. The
first thing to check is not the kernel.

The tail of a well behaved loop on an ordinary desktop is usually tens to
hundreds of microseconds. Bringing it under control needs a real time scheduling
policy, a reserved processor, locked memory and a kernel built for it, which is
the work the rest of this phase does.

## Build It

Implement in `exercise/solution.hpp`:

- `Histogram(bucket_width, bucket_count)`, allocating once.
- `record(value)`, incrementing the right bucket, counting overflows separately,
  and remembering the true worst value.
- `count()`, `worst()`, `overflows()`.
- `percentile(p)`, returning the upper edge of the bucket in which the pth
  percentile falls, for p from 0 to 100.
- `over(budget)`, how many samples exceeded a budget, which is the number a
  specification is actually written against.
- `LoopMonitor(period)`, whose `tick(now)` returns the interval and the lateness.

```
rcpp verify 07-03
```

The tests check the histogram arithmetic deterministically, with fed values and
fed timestamps. One further test runs a real loop and prints its histogram, so
you finish the lesson looking at the actual timing of your own machine.

## Use It

`perf sched`, `cyclictest` and `ftrace` are what production work uses, and
`cyclictest` in particular is the standard way to characterise a machine before
trusting it with a control loop. All of them produce this: a distribution, and
percentiles taken from it.

Building one yourself is worth the hour because a histogram inside your own
process measures the thing you care about, which is your loop, rather than the
machine in general. Every timing claim in this curriculum was produced this way.

## What Breaks First

- **A histogram that hides its own tail.** Samples beyond the last bucket folded
  into it, so the worst case reads as the top of the range. See `E-RT-0001`.
- **A percentile computed with integer arithmetic.** The index calculation
  truncates and the answer is systematically low. See `E-NUM-0011`.
- **An index that runs past the last bucket.** The overflow check must come
  before the array access, not after. See `E-CPP-0007`.

## Ship It

`Histogram` and `LoopMonitor` join `rc::rt`. From here, any claim about timing in
this curriculum is expected to come with a distribution rather than an average,
and this is the instrument that produces it.
