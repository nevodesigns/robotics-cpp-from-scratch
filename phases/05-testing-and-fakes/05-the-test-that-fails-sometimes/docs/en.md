# The Test That Fails Sometimes: Measuring Flakiness Instead of Rerunning

> The first version of this lesson's own test measured one method for a while
> and then the other, and reported the opposite of the truth.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 05-04, 03-05

## The Problem

A test goes red on the build server. Somebody presses the button again, it goes
green, and the change is merged.

That happens next week too, and the week after, and eventually the test is
marked as one to ignore or quietly deleted. Nobody can say how often it fails,
because nobody has ever tried to find out.

A flaky test is not a test that is wrong. It is a test whose result is a
**random variable**, and rerunning it takes one more sample and throws the
sample away.

## The Concept

### Count, do not rerun

```cpp
const rc::test::Trials trials = rc::test::repeat(200, [] { return check(); });
```

A number, and then a decision. One failure in three is a broken test or broken
code, and is usually easy to find once you know it is that common. One in a
thousand is a race, or a timeout, or a machine that changes, and needs an
entirely different search.

Rerunning hides the difference, because both look identical from the outside: a
failure, then a pass.

It also teaches the wrong lesson. Once a suite has one test known to need a
second attempt, every genuine failure gets a second attempt too, and the suite
stops being evidence about anything.

Do it **before** investigating, because the rate says which hunt to start. Do it
**again after fixing**, because "it passes now" is one sample and two hundred
passes is evidence.

### A timing distribution has a floor and no ceiling

The same small piece of work, timed four hundred times:

| | |
|---|---|
| fastest | 38.39 us |
| median | 39.50 us |
| mean | 42.31 us |
| 99th percentile | 95.00 us |
| worst | 105.53 us |

The median is **one microsecond** above the floor and the worst is nearly three
times it. Nothing can make the work faster than it is; a great many things can
make it slower.

So a threshold placed anywhere near the middle of that is a coin toss, and
taking the smallest of several samples moves the measurement toward the floor,
where it is repeatable:

```cpp
const double microseconds = rc::test::best_of(5, [] { return time_the_work(); });
```

A mean measures whatever else the machine was doing. A single sample measures
whether it happened to be doing it just then. The minimum is the closest any run
came to measuring the thing itself.

### And the machine does not stay still

Here is the measurement that decides what a benchmark may assert. The same work,
in blocks of two hundred, through one run:

| block | median | fastest |
|---|---|---|
| 0 | 41.21 | 38.38 |
| 1 | 43.11 | 38.38 |
| 2 | 45.23 | 43.99 |
| 4 | 48.79 | 46.25 |
| 7 | 48.79 | 47.47 |

Nineteen percent slower by the end, and **the fastest sample rose with it**. A
rising floor is not noise: it is the machine changing state, a clock speed
settling out of a boost or a thermal limit arriving.

Two consequences, and they are the whole lesson.

**A threshold calibrated in a warm up measures a machine that no longer exists**
by the time the test runs. No amount of taking the smallest of several undoes a
systematic shift.

**So never put an absolute time in a test.** The only assertion about a
benchmark that survives is a comparison measured **now**: A against B, in the
same run.

### Interleave, or measure nothing

And measure A and B alternately in one loop, not one block after the other.

This is not a refinement. The first version of this lesson's own test measured
two hundred samples of a single reading and then two hundred of a best-of-five,
compared their failure rates, and reported that the best-of-five was **worse**.
It was not. The machine had drifted between the two blocks, and the experiment
compared one method's minute against the other's.

Which is why the suite in this lesson prints its timing tables and asserts
almost nothing about them, keeping only the three relations that hold on any
machine: the fastest is not above the median, the worst is not below the 99th
percentile, and the mean is not below the fastest.

**A test about flaky tests must not be one.**

### Wait for the thing, not for a length of time

A fixed sleep is a guess about how long something else will take, and it is
wrong in both directions at once. Too short and the test fails whenever the
machine is busy, which is when a build server runs it. Too long and every run
pays for it, including the runs that did not need it. A machine four times
slower needs a sleep four times longer, and there is no number that is both.

```cpp
const bool ready = rc::test::wait_until([&] { return finished.load(); }, 5.0);
RC_CHECK(ready);
```

Measured on a worker doing a few hundred thousand additions, that returned in
**0.55 ms** against a deadline of five seconds.

That is the trade. A generous deadline costs nothing when things work, because
it is never reached, and it exists only so that a condition which never becomes
true **fails the test** rather than hanging the suite. A fixed sleep costs its
whole length every single time.

Make the deadline generous, since it is not a performance assertion. Check the
condition before the deadline, so something already true costs nothing. Poll
rather than spin, so a waiting test does not take a core from the work it is
waiting for. And assert that it did not time out, or the test carries on and
fails somewhere less informative.

## Build It

Implement `repeat`, `best_of` and `wait_until` in `exercise/solution.hpp`.

```
rcpp verify 05-05
```

`best_of` is tested against a sequence of known numbers rather than against a
clock, because a test of a timing tool must not itself depend on timing. The
timing tables are printed and left unasserted, for the reason above.

## Use It

**Measure the rate before investigating**, and again after fixing.

**Never retry to green in CI.** A retry hides the one number that would have
made the fault findable.

**Never assert an absolute time.** Assert a comparison measured in the same run.

**Interleave the things you are comparing.**

**Replace every sleep with a wait and a deadline**, and check the deadline was
not reached.

## What Breaks First

- **A test rerun until it passes.** See `E-TEST-0008`.
- **A threshold in a test, and a machine that moves.** See `E-TEST-0009`.
- **A sleep where a wait belonged.** See `E-TEST-0010`.

## Ship It

`Trials`, `repeat`, `best_of` and `wait_until` join `rc::test` beside the
framework, the leak checker and the generator. Every later lesson that has to
measure something on a shared machine has the tools, and the discipline, to do
it without shipping a test that needs a second attempt.
