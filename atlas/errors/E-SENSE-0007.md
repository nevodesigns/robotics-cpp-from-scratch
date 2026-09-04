id: E-SENSE-0007
title: A degenerate fit produces a nan that passes the range check
match: the nan that refusal exists to prevent
match: a fit that cannot be done says so rather than dividing
platforms: linux, windows
teaches: 15-02-precise-and-wrong
---

## Symptom

Every reading downstream of the sensor is `nan`, or `-nan`, or the robot simply
stops responding to it. The range check that was supposed to catch a bad reading
did not catch this one.

The calibration ran without complaint.

## Cause

Two things happened, and the second is the one worth remembering.

**The fit was impossible.** A least squares line divides by

```cpp
const double denominator = n * sum_rr - sum_r * sum_r;
```

which is the spread of the raw readings, and is exactly zero when they are all
the same. That happens for real reasons: the reference was never moved between
readings, the sensor saturated and reported its maximum every time, or the
device was disconnected and the driver returned its last value. The numerator is
zero as well, so the division is `0.0 / 0.0`, and that is a `nan` rather than an
infinity.

**The nan survived the check.** Every comparison involving a nan is false,
including `nan > 100.0` and `nan < 0.0`. So these two lines, which say the same
thing in English, do not do the same thing:

```cpp
if (value < 0.0 || value > 100.0) reject(value);    // lets the nan through
if (!(value >= 0.0 && value <= 100.0)) reject(value);  // catches it
```

From there it spreads. Adding to a nan, multiplying it by zero, and accumulating
it into an integrator all give a nan, and nothing brings the value back.

Worth knowing while reproducing this: MSVC will not compile a division by zero
it can fold at compile time, and reports `error C2124: divide or mod by zero`
where GCC and Clang accept the same line. Accumulate the sums from real data
rather than writing the constants out, which is what the program does anyway.

## Fix

Refuse the fit rather than performing it.

```cpp
if (denominator == 0.0)
  return rc::unexpected<CalibrationError>(CalibrationError::no_spread);
```

A calibration that cannot be computed is an ordinary outcome, not an exceptional
one, and returning it as a value means the caller has to look. This is what
`rc::expected` is for, and lesson 03-02 is where it was built.

Write range checks so that a nan fails them. Phrase the condition as what you
require and negate it, rather than listing what you reject:

```cpp
if (!(value >= lower && value <= upper)) reject(value);
```

That form rejects a nan because the requirement is not met, which is the answer
you wanted. And where a value comes in from outside your own code, say so
directly with `std::isnan`, which is the only comparison that works.
