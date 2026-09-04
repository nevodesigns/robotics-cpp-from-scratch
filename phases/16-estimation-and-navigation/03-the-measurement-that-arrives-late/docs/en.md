# The Measurement That Arrives Late: Fusing What Is Already Old

> The filter reported the same confidence in every run: the one that was 3 cm
> out and the one that was 59 cm out.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 16-02, 15-03

## The Problem

Lesson 16-01 built odometry and watched it drift. Lesson 16-02 stopped the drift
by fusing an absolute measurement with it. Neither asked when the absolute
measurement was taken.

Odometry arrives every 2 ms because it is two counters subtracted. An absolute
fix arrives every 100 ms and most of that was spent producing it: exposure,
transfer, detection, the pose solve. By the time the number exists the robot has
moved, and lesson 15-03 already measured what happens when you pretend otherwise.

Here that mistake is inside a filter, which makes it worse in a specific way.
The filter does not merely use the late number, it **converges on it**, and then
reports that everything is fine.

## The Concept

### Fusing a fix as though it were current

Forty seconds at 1 m/s, odometry at 500 Hz, an absolute fix good to 10 cm
arriving at 10 Hz. The only thing changed between runs is how old the fix is:

| fix age | fused as current | projected forward | rewound | filter claims |
|---|---|---|---|---|
| 0.000 s | 0.0325 | 0.0325 | 0.0325 | 0.0376 |
| 0.040 s | 0.0445 | 0.0339 | 0.0338 | 0.0376 |
| 0.080 s | 0.0782 | 0.0350 | 0.0350 | 0.0376 |
| 0.150 s | 0.1449 | 0.0369 | 0.0425 | 0.0376 |
| 0.300 s | 0.2930 | 0.0410 | 0.0533 | 0.0376 |
| 0.600 s | 0.5920 | 0.0494 | 0.0691 | 0.0376 |

Dead reckoning alone managed **0.4478 m** over the same run.

Read the first column against that number. A fix 600 ms old, fused as though it
were current, left the estimate worse than using no fix at all. The fusion was
not merely failing to help; it was actively dragging the estimate into the
measurement's past, and doing so with every correction.

Where that crossing point falls depends on how fast the odometry drifts, so it
moves. The direction does not.

### Moving the measurement to the present

The fix is right about somewhere. It is right about where the robot was, and the
filter already knows what the robot has done since, because that is the odometry
it has been integrating all along.

```cpp
const rc::nav::Estimate moved =
    rc::nav::project_forward(fix, motion_since_it_was_taken, motion_variance_since);
filter.correct(moved.value, moved.variance);
```

Two lines, and the second column of the table is what they buy: from 0.0325 m
with no delay to 0.0494 m at 600 ms, against 0.5920 for the naive version. The
price is a ring of doubles as deep as the longest latency you tolerate.

The variance grows too, because the motion you added was measured rather than
known. That half does not make the estimate more accurate, and with poor
odometry it costs a little: measured, 0.1807 m against 0.1778 m. What it buys is
that the filter's statement about itself stops being a fiction, which the next
section is about.

### The textbook answer, done the usual way

Rewinding is what the literature recommends: put the filter back to the moment
the measurement describes, correct it there, and replay forward.

The third column is that, written the way it is usually written first. It
replays the motion since. It does not replay the corrections since.

Up to 80 ms it matches the projection exactly. From 150 ms it is worse and stays
worse, and the turn is at the fix interval: fixes arrive every 100 ms, so a
rewind reaching further back than that lands before a correction it then throws
away.

A rewind is exact when it replays **everything** that happened in the interval.
That is a real technique with a real cost in bookkeeping. It is not a technique
you get by rewinding and replaying the odometry, and the difference between the
two is invisible until the latency exceeds the fix interval.

### The filter's confidence is not an accuracy figure

The last column is the same number in every row. `0.0376` when the estimate is
3 cm out, and `0.0376` when it is 59 cm out.

Nothing is wrong with the arithmetic. It answers a different question than the
one people read it as: *given these variances and this motion model, how
uncertain should I be?* The measurements being late is not in that question.
Neither is a slipping wheel, a reflecting beacon, or a variance somebody guessed
once and never revisited.

A covariance is a consistency figure. Reporting it as an accuracy specification
is the mistake in `E-NAV-0006`, and it is a comfortable one to make, because the
number is right there and it looks authoritative.

### What an innovation monitor sees, and what it does not

The one check available that needs nothing from outside the filter is the
innovation: the measurement minus the prediction, divided by the deviation the
filter itself expected of that difference.

```cpp
monitor.add(rc::nav::innovation_of(filter.estimate(), measurement));
```

It should sit near one. Here is what it actually reported, on the same naive
runs:

| fix age | steady 1 m/s | stop and go |
|---|---|---|
| 0.000 s | 0.921 | 0.921 |
| 0.080 s | 0.923 | 1.182 |
| 0.300 s | 0.927 | 2.679 |
| 0.600 s | 0.935 | 4.386 |

**At a constant speed it sees nothing.** The error goes from 3 cm to 59 cm and
the monitor does not move, because the estimate has settled into the
measurement's own past and the two now agree with each other perfectly. They are
consistent. They are both wrong.

Change the speed and the lag changes with it, and then the monitor reports
everything: 4.4 against a healthy 0.92.

That is the general lesson and it reaches well past estimation. **A consistency
check can only see an error that changes.** A robot driven at one speed in a
straight line will pass tests it should fail.

Note also that the healthy figure is 0.92 rather than 1.0. Compare a monitor
against a baseline you measured on a run you trusted, not against the number the
theory predicts.

## Build It

Implement `project_forward`, `innovation_of` and `ConsistencyMonitor` in
`exercise/solution.hpp`.

```
rcpp verify 16-03
```

The suite checks the three, then runs the same forty seconds six ways, then asks
the monitor what it noticed.

## Use It

**Timestamp the fix at its own moment**, not at the moment it was computed and
not at the moment it arrived. For a camera that is the exposure, which is
usually the one thing the pipeline does not carry forward.

**Keep motion history as deep as your worst latency**, and no deeper. It is the
only state projection needs, and knowing the depth means knowing the latency,
which is worth having written down for the reasons in `E-SENSE-0008`.

**Publish the monitor beside the covariance.** One of them can be wrong without
the other noticing, and an operator shown only the confident number is right to
believe it.

**Make the acceptance run change speed.** Accelerate, stop, turn. Everything in
this lesson that was invisible was invisible because something was held constant.

## What Breaks First

- **A fix fused as though it described the present.** See `E-NAV-0005`.
- **A covariance read as an accuracy figure.** See `E-NAV-0006`.
- **A consistency check that cannot see a steady error.** See `E-NAV-0007`.

## Ship It

`project_forward`, `innovation_of` and `ConsistencyMonitor` join `rc::nav` as
`rc/nav/delayed.hpp`. The filter from 16-02 can now be given measurements from
the real world, where nothing arrives when it happened, and it can be asked
whether it is telling the truth.
