# The Sensor That Is Sometimes Wildly Wrong: Outliers, the Median, and What Each Costs

> A mean moves by how wrong a bad reading is. A median moves by nothing. Find out
> what that buys, where it stops, and what a filter that only removes spikes does
> instead.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 15-01

## The Problem

Lesson 15-01 measured what smoothing costs: every filter is a delay. That was about a
sensor with steady noise. Some sensors are not like that. They are right, within a little
noise, nearly all of the time, and now and then they report something absurd: a distance
of a metre in a room where nothing moved, a temperature that jumps, an encoder that
reports a million counts once.

An average over such a sensor is still thrown off. Averaging is the right tool for noise
and the wrong one for this, and the reason is exact.

## The Concept

### A mean counts how wrong, a median only which side

A mean moves by the size of a bad reading over the number of readings. A reading of 105
among readings of 5, in a window of nine, moves it by 100 over 9, which is 11.1, for nine
readings. The worse the reading the further it moves, and a sensor that is occasionally
wildly wrong has no bound on how wrong.

A median is the middle of the sorted window. It counts which side of the middle a reading
falls, and not how far, so the same spike moves it by nothing.

The test checks the streaming median against the plain method, sorting a copy, for six
window sizes, while the window fills and after, 1800 readings in all, including readings
that repeat, which is the awkward case for a middle value.

### What each costs

A sine of amplitude 1 with noise of 0.05, and 211 of 20000 readings wrong by 50 to 100,
compared with the truth:

| filter | rms error |
|---|---|
| none | 7.681 |
| mean of nine | 2.519 |
| median of nine | 0.092, or 0.025 allowing for its delay |
| Hampel, nine, three sigma | 0.053 |

The mean turned 7.7 into 2.5, still fifty times the noise. The median is nearly right,
and almost all of what is left is that it comes out four samples late: compared against the
truth four samples earlier, its error is 0.025, which is below the noise that went in,
because it also averages. See `E-SENSE-0014`.

### The Hampel filter does something else

A Hampel filter does not smooth. It looks at each reading against the median of its
window, measures how far that is in units of the window's own spread, and replaces the
reading with the median only if it is more than a threshold away. Anything else comes out
exactly as it went in.

The spread is the median absolute deviation, times 1.4826 so that it estimates a standard
deviation for ordinary noise. Of 2000 clean readings, 61 were replaced, which is 3 percent
and more than the three in a thousand that three known standard deviations would give,
because nine readings estimate the spread poorly. The rest were not touched at all, not
smoothed, not rounded, not shifted.

On the sine its error was 0.053, which is the 0.05 of noise that went in. It removed every
spike and left the noise alone, with no delay. Which of the two is right depends on whether
the noise or the spikes are the problem.

### Where a median stops

A median is right as long as the bad readings are fewer than half the window. A burst of
consecutive readings of 100 among zeros, a window of nine:

| burst length | worst output |
|---|---|
| 1 | 0 |
| 2 | 0 |
| 3 | 0 |
| 4 | 0 |
| 5 | 100 |
| 6 | 100 |

Four are outvoted and the fifth is a majority. The limit is half the window, rounded down.
To survive a burst of `b`, the window needs at least `2b + 1`, and it costs `b` samples of
delay. A burst longer than the window is not an outlier, it is a fault, and wants the
staleness check of lesson 15-04. See `E-SENSE-0015`.

### The price of not passing a spike

On the first reading of a step, a step and a spike are the same thing: one number far from
the rest. The filter cannot know which until the readings after it say. It waits until half
the window agrees.

A step from 0 to 1 on a perfectly quiet sensor was held back by a Hampel filter of nine for
**4 readings**. A median is the same: its step arrives 4 samples late, but then rises in a
single sample where a mean takes 8. That is the same four as the breakdown point, because
they are one fact: half a window.

A quiet sensor makes it worse. A window with no spread at all has a median absolute
deviation of exactly zero, so a threshold of three times it is zero, and every change,
however small and however real, is infinitely far from it. A change of 0.003 on a sensor
with a resolution of 0.001 was rejected for 4 readings with no floor on the spread and for
none with a floor of 0.001. The floor is the one thing the window cannot know, the least
the sensor can resolve, and the caller does. See `E-SENSE-0016`.

## Build It

`MedianFilter` and `HampelFilter` in `exercise/solution.hpp`. `middle_of` is given: it uses
`std::nth_element` to put the middle value where it belongs without sorting the rest, which
is all a median needs and is cheaper than a sort.

```
rcpp verify 15-07
```

Two tests check the filters against the definition by sorting. The rest are the tables
above.

## Use It

`rc/core/outlier.hpp` joins `rc::core`, beside the filters of lesson 01-04 and 15-01.

```cpp
rc::core::HampelFilter reject(9, 3.0, sensor_resolution);
const double cleaned = reject.update(raw);
```

Then a mean or a low pass filter for the noise, if it needs one, after the spikes are gone
and not before: a spike that reaches an average has already done its damage.

**What it does not do.** It does not tell you the sensor has failed. `rejected()` counts
what it replaced, and a count that climbs is information that the staleness check and the
fault handling should have. A filter that quietly repairs a dying sensor is hiding it. It
also works on one number at a time: a vector reading, a position in a plane, wants the
outlier judged on the whole vector, not each part separately.

## What Breaks First

- **A mean over a sensor with spikes.** It is moved by how wrong they are. See
  `E-SENSE-0014`.
- **A burst longer than half the window.** The median is the burst. See `E-SENSE-0015`.
- **A real step, rejected as a spike.** The filter has to wait for half a window, and a
  quiet sensor needs a floor on the spread. See `E-SENSE-0016`.

## Ship It

`rc/core/outlier.hpp` holds `middle_of`, `MedianFilter` and `HampelFilter`.

Lesson 15-04 says what to do when a sensor has stopped. This says what to do when it is
wrong without stopping, and the two together are the front end of anything that listens.
