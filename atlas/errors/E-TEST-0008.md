id: E-TEST-0008
title: A test that is rerun until it passes
match: counting failures instead of rerunning until green
platforms: linux, windows
teaches: 05-05-the-test-that-fails-sometimes
---

## Symptom

A test goes red on the build server. Somebody presses the button again and it
goes green, and the change is merged. It happens again next week, and the week
after, and eventually the test is marked as one to ignore or deleted.

Nobody can say how often it fails, and nobody has ever tried to find out.

## Cause

A flaky test is not a test that is wrong. It is a test whose result is a
**random variable**, and rerunning it takes one more sample and throws the
sample away.

That discards the only information anybody has. One failure in a thousand and
one failure in three are very different faults with very different causes, and a
red run followed by a green one cannot tell them apart. Worse, both look
identical from the outside: a failure, then a pass.

The rerun also teaches everybody the wrong lesson. Once a suite has one test
that is known to need a second attempt, every genuine failure gets a second
attempt too, and the suite stops being evidence about anything.

## Fix

Measure the distribution instead of sampling it.

```cpp
const rc::test::Trials trials = rc::test::repeat(200, [] { return check(); });
std::cout << trials.failures << " of " << trials.runs
          << " (" << trials.failure_rate() * 100.0 << "%)\n";
```

A number, and then a decision. One in three is a broken test or broken code and
is usually easy to find once you know it is that common. One in a thousand is a
race, or a timeout, or a machine that changes, and needs a different search.

Three things that follow.

**Do it before investigating.** The failure rate tells you which kind of hunt to
start, and it takes a minute.

**Do it again after fixing.** "It passes now" is one sample. Two hundred passes
is evidence, and the same command produces it.

**Never rerun to green in CI.** A retry hides the rate, which is the one number
that would have made the fault findable. If a suite has to retry to be usable,
the retry count is a measurement worth publishing rather than a setting worth
raising.

What the rate is usually caused by is time or randomness. Randomness is
`E-TEST-0007`, fixed by seeding and printing the seed. Time is `E-TEST-0009`.
