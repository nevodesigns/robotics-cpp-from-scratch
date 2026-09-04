id: E-SENSE-0001
title: A filter that lies for its first N readings
match: the average is over the readings there are
match: the oldest reading leaves the window
platforms: linux, windows
teaches: 15-01-every-filter-is-a-delay
---

## Symptom

A robot that does something alarming in the first second after switching on, and
behaves perfectly ever afterwards.

A distance sensor reads near zero and climbs to the truth. A robot parked
against a wall believes the wall is far away and drives at it. Restarting
reproduces it exactly, which at least makes it findable.

## Cause

The average was divided by the **window** rather than by the number of readings
in it:

```cpp
double total = 0.0;
for (std::size_t i = 0; i < window_; ++i) total += samples_[i];
return total / static_cast<double>(window_);   // the bug
```

Before the window is full those are different numbers. The unfilled slots
contribute their initial value, usually zero, so the first reading comes out as
one Nth of the truth and the output climbs over N samples.

At 500 Hz with a window of 64 that is an eighth of a second of a sensor reading
low. It is short enough to miss in testing and long enough for a robot to start
moving on it.

There is a second version of the same mistake, with a different shape. A filter
whose oldest reading never leaves is a running mean over **all** of history: it
starts correct, becomes steadily less responsive, and after an hour barely moves
when the sensor does.

## Fix

Divide by what is actually there, and drop what has fallen out:

```cpp
samples_[next_] = reading;
next_ = (next_ + 1) % window_;
if (filled_ < window_) ++filled_;

double total = 0.0;
for (std::size_t i = 0; i < filled_; ++i) total += samples_[i];
return total / static_cast<double>(filled_);
```

The first reading then comes straight through, which is the right answer: with
one sample, the best estimate is that sample.

Two things worth having beside it.

**Say whether it is warm.** A caller that needs the full noise reduction can ask
`warm()` and wait; one that just needs a number can use it immediately. Both are
reasonable and only one of them is safe to assume.

**Guard the window.** A window of zero divides by zero, and the value usually
arrives from a configuration file rather than from a literal.

The alternative fix, priming the window with the first reading, gives the same
answer here and stops being equivalent as soon as the filter is a median or
anything else non linear. Dividing by the count is the one that generalises.
