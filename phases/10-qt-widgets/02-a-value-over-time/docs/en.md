# A Value Over Time: The Chart That Lies About Steady Signals

> A battery reading steady to within two microvolts, drawn on an axis fitted to
> it, swings from the top of the chart to the bottom. Nothing is wrong with the
> battery.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 03-05, 00-07

## The Problem

Lesson 00-07 drew a path: two numbers, plotted against each other. This is the
other chart every robot needs, and it is a different problem.

A **strip chart** shows one value against time. Battery voltage, a joint angle,
loop lateness, the error a controller is working on. It is the instrument you
watch while something is running, and every later phase of this curriculum has
one thing it wants to watch.

It is also the chart with the most ways to mislead, and the two worst are
invisible unless somebody points them out. Here is one of them:

```text
    12.0000 |############################################################
    12.0000 |
    12.0000 |
    12.0000 |
    12.0000 |############################################################
    12.0000 |
    12.0000 |
    12.0000 |
```

That is a battery holding twelve volts to within two microvolts. The chart says
it is oscillating wildly. The axis labels are the only thing on the screen
telling the truth, and they are the part nobody reads.

## The Concept

### The window is time, not a number of samples

The obvious rolling buffer keeps the last N samples:

```cpp
samples.push_back(sample);
if (samples.size() > 500) samples.erase(samples.begin());   // the trap
```

Five hundred samples is fifty seconds of history at 10 Hz and half a second at
1000 Hz. The chart is labelled in seconds and scaled by sample count, so the
same event is drawn a hundred times wider on one machine than another, and two
runs of the same signal cannot be compared.

Bound it by **age**, and keep a capacity as a separate guard:

- **Age is the policy.** A chart is about what is happening now, and a window of
  seconds is what a reader means by that.
- **Capacity is the guard.** It is what stops a signal arriving faster than
  anybody planned from growing the buffer without limit, so a program running
  for a week uses the same memory as one running for a minute.

Two bounds, two different questions, and both are needed.

### Draw the window, not the samples

The x axis spans the window, always, whether samples arrived or not.

That matters because of what it shows when they did not. If the width is the
sample count, four seconds of silence is drawn as one step between two adjacent
points, and a link that stopped talking looks identical to one that did not. If
the width is the window, the silence is forty percent of the chart and is the
first thing you see.

A gap is usually the most important thing on a telemetry chart. It should be
impossible to miss, and the mapping is what decides whether it is.

### An axis fitted to the data collapses

Fitting the value axis to the minimum and maximum of the data is the obvious
thing, and it is right up to the point where the data stops moving.

Every real measurement moves in its last digit. An analogue to digital converter
has a least significant bit, and it is never perfectly still. Fit an axis to
that and it expands until the wobble fills the chart, so a change of one part in
ten million is drawn exactly as large as a change of one volt.

Measured, on the numbers at the top of this page: a signal with a span of
0.000002 volts occupies more than fifteen of twenty rows.

**Padding does not fix it**, and understanding why is the point. Padding adds a
fraction of the span, and a fraction of nearly nothing is nearly nothing:

```cpp
padded(Range{7.0, 7.0}, 0.5)   // still a span of zero
```

What fixes it is a **minimum span**:

```cpp
Range at_least(const Range& range, double minimum_span);
```

And the number you pass is a judgement about the signal, not a constant to copy.
For a battery, a tenth of a volt is a change worth seeing and a microvolt is not.
For a joint angle it might be a degree. Writing it down states what counts as a
change, which is something the chart cannot know and its author can.

Then check it from both sides, because either check alone is easy to satisfy
wrongly:

- A steady signal must **not** fill the chart.
- A real change must still be **visible** on the same rule.

A minimum so large that a genuine event disappears has traded one lie for
another.

### The cost of the simple container

Dropping from the front of a `std::vector` moves everything after it, which
lesson 03-03 measured. At a few hundred samples that is invisible, and the
moment it is not, the lock free ring buffer from lesson 07-02 is waiting.

Choosing the simple one first and knowing exactly where its limit is beats
reaching for the clever one before there is a reason. That is a judgement worth
practising on something this small.

## Build It

Implement in `exercise/solution.hpp`:

- `Series`, a rolling window bounded by age and guarded by capacity.
- `range_of`, `padded`, `at_least`, the value axis and the two things that go
  wrong with it.
- `place_sample`, the mapping, with the same flip every surface in this
  curriculum uses.

```
rcpp verify 10-02
```

The suite draws two charts and prints them: the same steady signal on an axis
fitted to it and on one with a floor under its span. Run it and look at both.

## Use It

Point it at anything that changes. The lateness histogram from lesson 07-03
tells you about a distribution; this tells you when, which is the question you
have when something went wrong once.

The next lesson puts the same series behind a Qt widget that redraws as data
arrives. The arithmetic here does not change, which by now should be unsurprising.

## What Breaks First

- **A window bounded by count.** The chart covers a different amount of time on
  every run. See `E-PLOT-0004`.
- **An axis fitted to a steady signal.** It fills the chart with the last digit
  of the reading. See `E-PLOT-0005`.
- **An axis with no span at all.** Dividing by it puts every sample at
  infinity. See `E-PLOT-0003`.

## Ship It

`Series` and the axis rules join `rc::plot`, next to the path mapping from
phase 00. That module is now where this curriculum turns numbers into pictures,
whatever the picture is drawn on.
