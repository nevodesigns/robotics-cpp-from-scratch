id: E-TEST-0009
title: A threshold in a test, and a machine that does not stay still
match: what a single timing measurement is actually measuring
match: the machine is not the same machine ten seconds later
match: the smallest of several, on numbers that are known
platforms: linux, windows
teaches: 05-05-the-test-that-fails-sometimes
---

## Symptom

A performance test that passes on a laptop and fails on the build server, or
passes in the morning and fails in the afternoon. The threshold gets raised, the
test passes for a month, and then it fails again.

Eventually the number in the test has nothing to do with the code and everything
to do with the last machine that complained.

## Cause

Two facts about timing, and the second is the one that decides the answer.

**A timing distribution has a floor and no ceiling.** Nothing can make the work
faster than it is; a great many things can make it slower. Measured on one small
piece of work, four hundred times:

| | |
|---|---|
| fastest | 38.39 us |
| median | 39.50 us |
| mean | 42.31 us |
| 99th percentile | 95.00 us |
| worst | 105.53 us |

The median is one microsecond above the floor and the worst is nearly three
times it. A threshold placed anywhere near the middle of that is a coin toss.

**The machine changes while you measure it.** The same work, in blocks of two
hundred through one run:

| block | median | fastest |
|---|---|---|
| 0 | 41.21 | 38.38 |
| 2 | 45.23 | 43.99 |
| 4 | 48.79 | 46.25 |
| 7 | 48.79 | 47.47 |

Nineteen percent slower by the end, and **the fastest sample rose with it**, so
it is the machine changing state rather than noise: a clock speed settling out
of a boost, a thermal limit, a cache filling.

So a threshold calibrated during a warm up is measuring a machine that no longer
exists by the time the test runs.

## Fix

**Take the smallest of several samples.**

```cpp
const double microseconds = rc::test::best_of(5, [] { return time_the_work(); });
```

Noise can add time and never remove it, so the minimum is the closest any run
came to measuring the thing itself. A mean measures whatever else the machine
was doing and a single sample measures whether it happened to be doing it just
then.

**Never put an absolute time in a test.** No amount of taking the smallest of
several undoes a systematic shift, and the table above is that shift. The only
assertion about a benchmark that survives is a **comparison measured now**: A
against B, in the same run.

**Measure A and B interleaved, not one block after the other.** This is not a
detail. The first version of lesson 05-05's own test measured two hundred
samples of one method and then two hundred of the other, and reported the
opposite of the truth, because the machine had drifted between the blocks.

**And be honest about what cannot be asserted.** That lesson's suite prints its
timing tables and asserts almost nothing about them, keeping only the three
relations that hold on any machine: the fastest is not above the median, the
worst is not below the 99th percentile, and the mean is not below the fastest. A
test about flaky tests must not be one.
