# Arrays and Spans: Many Readings at Once

> One sensor reading is a rumour. Ten of them are evidence.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 01-03

## The Problem

A distance sensor pointed at a wall one metre away reports 1.00, 1.02, 0.98,
1.01, and then 4.7, because a reflection went somewhere strange for one reading.
Feed that raw stream to a controller and the robot will flinch.

Fixing this needs two things you do not have yet: a way to hold many readings at
once, and a way to pass all of them to a function without copying them.

## The Concept

### An array is many values side by side

```cpp
double readings[5] = {1.00, 1.02, 0.98, 1.01, 4.70};
```

Five doubles laid out in memory back to back. `readings[0]` is the first,
`readings[4]` is the last. There is no `readings[5]`, and asking for it does not
raise an error. It reads whatever bytes happen to sit after the array, which may
look like a plausible number. This is the single most dangerous property of the
language, and it is why bounds get checked by you, deliberately, every time.

`std::vector<double>` is the growable version: it knows its own size, it can
change size, and it cleans up after itself. Prefer it unless you have a reason
not to.

### A span is a view, not a copy

A function that averages readings should not care whether they came from a plain
array or a vector, and it should not copy them. `std::span` describes exactly
that: a pointer to the first element and a count, nothing more.

```cpp
double average(std::span<const double> samples);
```

Call it with a plain array or with a vector, and neither copies. The `const`
means the function reads and does not modify.

A span does not own anything. If the data it points at goes away, the span is
left pointing at nothing, and using it is undefined behaviour. Never store a span
that outlives the thing it views.

### Two filters worth knowing

A **moving average** replaces each reading with the mean of the last n. Smooth,
cheap, and it drags outliers along with it: one reading of 4.7 in a window of
five still lifts the answer by most of a metre.

A **median** filter sorts the window and takes the middle value. One wild
reading in five does not move the middle at all. Medians cost a sort and are what
you want against spikes.

Real perception stacks use both, at different stages, for exactly these reasons.

## Build It

In `exercise/solution.hpp`:

- `mean(std::span<const double> samples)` returns the average, and 0.0 for an
  empty span.
- `median(std::span<const double> samples)` returns the middle value, averaging
  the two middle values when the count is even, and 0.0 when empty. Copy before
  sorting: a function taking a read only view must not reorder the caller's data.
- `moving_average(std::span<const double> samples, int window)` returns a vector
  the same length as the input, where each entry is the mean of that reading and
  the ones before it, up to window readings. Return an empty vector when window
  is less than one.

```
rcpp verify 01-04
```

## Use It

`std::span` arrived in C++20 and is the modern way to pass a contiguous block.
Before it, this signature was a pointer and a length, which is what the C
libraries you will meet in phase 08 still use. Knowing that a span is exactly a
pointer and a length is what makes converting between the two obvious rather than
frightening.

## What Breaks First

- **Reading one past the end of an array.** Loops run from zero to less than the
  count. See `E-CPP-0007`.
- **A span that outlived its data.** You returned a span pointing at a local
  vector, and the vector died at the closing brace. See `E-MEM-0001`.
- **Sorting through a const span.** A read only view cannot reorder the source.
  Copy into a vector first. See `E-CPP-0009`.

## Ship It

`mean`, `median` and `moving_average` become the first filters in `rc::core`.
Phase 15 puts real sensor data through them, and phase 16 replaces them with an
estimator that knows what it is measuring.
