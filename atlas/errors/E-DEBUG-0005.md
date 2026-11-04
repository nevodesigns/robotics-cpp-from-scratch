id: E-DEBUG-0005
title: A mean that hides the iteration which missed the deadline
match: calls accumulate across iterations and the worst one is kept
platforms: linux, windows
teaches: 04-05-which-part-is-slow
---

## Symptom

Every number in the timing report is comfortably inside budget and the robot
stutters anyway. A loop with a one millisecond deadline reports a mean of 280
microseconds for its slowest stage. Somebody concludes the problem is
elsewhere.

## Cause

A mean answers how much work there was in total. It does not answer whether
any single pass was late, and lateness is not an average.

Measured over five iterations of one section:

| | microseconds |
|---|---|
| mean | 282 |
| worst | **900** |

Four passes near 110 and one at 900. The mean is a third of the worst, and the
900 is the one that missed.

This is the same shape as lesson 05-05, which measured a distribution with a
floor and no ceiling: the fastest sample was 38 microseconds, the median 39,
and the worst 105. Timing distributions are not symmetric, so the middle tells
you very little about the tail, and the tail is what a deadline cares about.

A reset that only moves a counter makes it worse, by carrying the previous
run's totals into the next one. Writing this lesson's artifact, a `reset` that
cleared the count and left the slots alone meant the next section registered
inherited the old calls and total. The suite caught it; nothing else would
have.

## Fix

**Keep the worst alongside the total.** It is one comparison per measurement
and it is the number that corresponds to a missed deadline.

**Report calls as well**, so a mean can be computed and so a section with an
implausible call count is visible, which is what a full section table looks
like when it has run out of slots.

**Compare the worst against the budget, not the mean.** A mean under budget
with a worst over it is a loop that fails occasionally, which is the hardest
kind of failure to chase and the subject of lesson 05-05.

**Keep a histogram when the tail matters.** `rc::rt::Histogram` from lesson
07-03 is the tool once you need percentiles rather than just the maximum.

**Clear the data, not just the count, when you reset.** Anything else leaves
stale totals for the next registration to inherit, which looks like a section
that was slow before you started measuring.
