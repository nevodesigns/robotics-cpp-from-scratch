id: E-PLOT-0004
title: A rolling chart that covers a different amount of time on every run
match: the window covers the same amount of time at any sample rate
match: samples older than the window are dropped
platforms: linux, windows
teaches: 10-02-a-value-over-time
---

## Symptom

Two charts of the same signal cannot be compared. One appears to show a slow
drift over a minute, the other a fast wobble over a second, and they came from
the same code watching the same thing.

The difference is usually a sample rate that changed for an unrelated reason: a
faster loop, a device that started reporting more often, a debug build.

## Cause

The rolling window was bounded by **count** rather than by **age**:

```cpp
samples.push_back(sample);
if (samples.size() > 500) samples.erase(samples.begin());   // the bug
```

Five hundred samples is five hundred samples. At 10 Hz that is fifty seconds of
history, and at 1000 Hz it is half a second, and the chart says nothing about
which it is currently showing.

The x axis is then meaningless: it is labelled in time and scaled by sample
count, so the same physical event is drawn a hundred times wider on one machine
than another.

## Fix

Drop by age, and keep the count as a separate guard:

```cpp
void add(double time, double value) {
  samples_.push_back(Sample{time, value});

  const double oldest_wanted = time - window_;
  std::size_t drop = 0;
  while (drop < samples_.size() && samples_[drop].time < oldest_wanted) ++drop;
  if (drop > 0) samples_.erase(samples_.begin(), samples_.begin() + drop);

  if (samples_.size() > capacity_) { /* trim to capacity as well */ }
}
```

Both bounds are needed and they answer different questions.

**Age is the policy.** A chart is about what is happening now, and a window of
seconds is what a reader means by that.

**Capacity is the guard.** It is what stops a signal arriving faster than
anybody expected from growing the buffer without limit, so a program running for
a week uses the same memory as one running for a minute.

Then draw the x axis from the **window**, not from the samples that happen to
have arrived. A chart whose width is the sample count spreads a gap in the data
evenly and hides it. A chart whose width is the window leaves the gap where it
belongs, which is often the most important thing on it.
