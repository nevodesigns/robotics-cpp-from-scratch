# Classes: A Thing That Cannot Be Wrong

> A struct is a bag of values that anybody can put anything into. A class is a promise about what is true, enforced by the only code allowed to change it.

**Type:** Build
**Time:** about 105 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 02-06

## The Problem

A robot watches a temperature sensor and wants a running mean, the highest and
lowest readings, and some measure of how noisy the signal is.

Written as a struct, it goes wrong within a week:

```cpp
struct Stats {
  int count;
  double mean;
  double lowest;
  double highest;
};
```

Somebody updates `mean` and forgets `count`. Somebody reads `lowest` before any
reading has arrived and gets whatever was in memory. Somebody writes a new value
into `highest` directly, because it was quicker than adding a function. Every one
of those is legal, and the type cannot object, because a struct makes no
promises.

## The Concept

### An invariant is a promise about the object

An **invariant** is something that is true about an object from the moment it is
created until it is destroyed. For running statistics:

- `count` is never negative.
- If `count` is zero, no reading has been seen and the mean is meaningless.
- If `count` is greater than zero, `lowest` is at most `highest`, and the mean
  lies between them.

The whole purpose of a class is to make those statements impossible to break.
Two mechanisms do it:

**Private data.** Members that outside code cannot touch:

```cpp
class RunningStatistics {
 public:
  void add(double value);
  int count() const;
 private:
  int count_ = 0;      // nothing outside this class can change these
  double mean_ = 0.0;
};
```

**A complete set of operations.** If the only way to change the object is
`add()`, and `add()` maintains every invariant, then the invariant holds after
every possible sequence of calls. Not because everybody remembered, but because
there is no other route.

### const says which functions may not change the object

```cpp
int count() const;      // promises not to modify the object
void add(double value); // may modify it
```

`const` on a member function is checked by the compiler, and it does two useful
things. It documents at a glance which half of the interface is safe to call on
anything, and it means the object can be passed as `const&` and still be asked
questions. A class where the reading functions are not marked `const` is
annoying to use and will be fixed by whoever meets it next.

The rule is simple: **if a member function does not change the object, mark it
`const`.**

### Computing a mean without losing precision

The obvious way to track a mean is to keep a running total and divide. It is
also the way that goes wrong.

Suppose readings are around 1,000,000 and vary by a hundredth. Add ten thousand
of them into a `double` total and the small variations fall off the bottom of
the representation, because a double has about fifteen significant digits and
they are being spent on the large constant part. The mean comes out plausible,
and the variance comes out as zero or negative, which is impossible.

Welford's method updates the mean by the correction each new reading implies,
rather than accumulating a large total:

```
count   = count + 1
delta   = value - mean
mean    = mean + delta / count
delta2  = value - mean            (using the new mean)
m2      = m2 + delta * delta2     (for variance)
```

Every quantity involved stays the size of the data rather than the size of the
sum, so precision is not spent on a constant offset. It is barely longer than
the naive version and it does not fail. Lesson 00-03 warned that numbers lie;
this is the standard defence.

## Build It

Implement `RunningStatistics` in `exercise/solution.hpp`:

- `add(double value)` updates count, mean, lowest, highest, and the accumulator
  needed for variance, using Welford's method.
- `count()`, `mean()`, `lowest()`, `highest()`, `variance()`, and
  `standard_deviation()`, all `const`.
- With no readings, count is zero and every other answer is 0.0 rather than
  undefined.
- Variance uses the sample form, dividing by `count - 1`, and is 0.0 when fewer
  than two readings have been seen.
- `reset()` returns the object to its starting state.

```
rcpp verify 03-01
```

**It will not compile at first, and that is the first thing to fix.** One test
asks for the mean through a `const RunningStatistics&`, and the reading
functions have no `const` on them yet, so the compiler refuses. That error is
`E-CPP-0011` and it takes one word per function to clear. Fix it before looking
at anything else, then the rest of the tests start telling you about Welford's
method.

One test feeds readings around one million and requires the variance to come out
right. The naive running total fails it, which is the point.

## Use It

The standard library gives you `std::accumulate` and `std::minmax_element` for
data you already hold, and lesson 03-03 uses them. They need the whole set at
once.

This class is for the other case, which is most of robotics: readings arriving
one at a time, forever, with no room to keep them all. That is called an online
algorithm, and Welford's is the canonical example.

## What Breaks First

- **The compiler refuses to call a reading function on a const object.** The
  function is not marked `const`. See `E-CPP-0011`.
- **Variance comes out zero or negative on large values.** A running total lost
  the small variations. Negative variance is impossible and is a reliable signal
  of exactly this. See `E-NUM-0010`.
- **Statistics passed to a function do not update.** It took the object by value
  and updated a copy. See `E-CPP-0006`.

## Ship It

`RunningStatistics` joins `rc::core` and is used by every later lesson that
measures something: sensor noise in phase 15, loop jitter in phase 07, and
controller error in phase 14.
