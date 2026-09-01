id: E-TEST-0004
title: The boundary that sampling never lands on
match: the checks reject a limiter that is wrong at exactly one step away
match: exactly one step
platforms: linux, windows
teaches: 05-03-the-test-that-catches-it
---

## Symptom

A bug that appears rarely and cannot be reproduced on demand. It survives a
thorough test suite, including one that runs thousands of randomly generated
inputs.

When it is finally caught, the trigger is embarrassingly specific: a distance
that happened to equal the step size exactly, a buffer that happened to be
exactly full, a timestamp that happened to land on the boundary of a window.

## Cause

Behaviour changes at a boundary, and the boundary is a single point in a
continuous range.

A rate limiter does two different things depending on whether the remaining
distance is more than one step or not, so the case where it is **exactly** one
step is where those two behaviours meet, and it is where an implementation stops
being described by the cases either side of it.

Random sampling almost never lands there. Measured in lesson 05-03: an
implementation wrong only when the distance equals the step exactly is missed by
a check that sweeps a hundred and one evenly spaced starting positions, and by
every other property in the suite. The only checks that catch it are the two
that ask for that case by name.

The same shape, in the places it actually appears:

- A ring buffer at exactly full, and at exactly empty. See `E-THREAD-0004`.
- A payload of exactly the size of the storage. See `E-IO-0004`.
- An angle of exactly pi, which is the same heading as minus pi. See
  `E-NUM-0006`.
- A timeout at exactly the timeout.

## Fix

Choose the boundaries deliberately, and write each as its own check with a name
that says which boundary it is.

The way to find them is to read the implementation for the places it **decides**
something. Every comparison is a boundary:

```cpp
if (std::fabs(remaining) <= max_step) return target;   // here
```

That line produces three cases worth testing, not one: less than, exactly, and
more than. Two of them are usually obvious and the third is this entry.

Random and property based testing are worth having as well, and they find things
a person would not have thought of. They are not a substitute for the chosen
boundary, because the probability of generating it is nearly zero and the
consequence of missing it is a bug that only appears in the field.

The cheap habit: whenever a test uses a value near a limit, add one at the limit.
