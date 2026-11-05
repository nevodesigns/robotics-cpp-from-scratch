# A Number That Changed Type on the Way: Casts, Truncation, and What Fits

> Not a number, infinity, and a number one too large all become -2147483648
> when cast to an int, which is also a reading a real sensor can produce.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 00-04, 01-07, 01-09

## The Problem

You have been writing `static_cast` since lesson 00-04, where it appeared in one
line to make a division fractional. It has appeared in most lessons since, and
nobody has said what it does.

It is a conversion, and a conversion is a number changing type. That happens
all the time in a robot: an ADC count becomes volts, a position becomes a grid
cell, a timestamp becomes a plain `int`, a count is sent as a float. The
language does most of these without being asked and says nothing. Most of the
time it is right. The times it is not, the wrong answer is plausible, which is
worse than a crash.

This lesson is placed here, after phase 00 already used casts, because a
conversion lesson needs the unsigned and overflow lessons behind it. That is an
ordering oddity and not a gap.

## The Concept

### An implicit conversion is a decision nobody made

```cpp
double d = 3.7;
int y = d;                      // compiles, y is 3, and nothing says so
int z = static_cast<int>(d);    // compiles, y is 3, and someone decided
```

Measured: the first line produced no warning on gcc 11 or clang 14 under
`-Wall -Wextra`. `-Wconversion` reports it on both, and is noisy enough that
lesson 04-08 is about deciding whether to believe it.

A `static_cast` changes nothing about what happens. It changes what the reader
knows: somebody saw the loss and meant it. That is all a cast is for. It is not
a way to make a warning stop.

### The fraction goes toward zero

```
   value   static_cast   +0.5 trick   floor_to_int   round_to_int
     2.9            2            3              2              3
    -2.9           -2           -2             -3             -3
     0.5            0            1              0              1
    -0.5            0            0             -1             -1
     2.5            2            3              2              3
    -2.5           -2           -2             -3             -3
```

A cast to `int` is neither rounding nor flooring. It discards the fraction
toward zero, so for a positive number it floors and for a negative number it
does the opposite.

### The usual repair is wrong below zero

Adding a half before casting rounds positive numbers correctly, which is why it
survives every test written with positive numbers:

```
  601 values from -3.00 to 3.00 in steps of 0.01:
    the +0.5 trick disagrees with round_to_int   251
    of those, on a positive value                0
```

251 disagreements, and not one on a positive value. `round_to_int` rounds halves
away from zero, so 2.5 is 3 and -2.5 is -3.

`E-NUM-0026`.

### Both sides of zero in one cell

A map of one metre cells and twenty positions from -1.0 to 0.9:

```
                  in cell 0   in cell -1
    static_cast          19            1
    floor_to_int         10           10
```

By cast, everything from -0.9 to 0.9 is cell 0. The cell at the origin is twice
the size of every other, and a robot standing at -0.4 is told it is at +0.4.
Lesson 16-05 meets the same trap in a larger program.

A map wants **floor**, because the cell of -0.5 is the one below zero.

### A number that does not fit still gets an answer

This is the one that costs most. The cast of a `double` to an `int` when the
value does not fit is **undefined behaviour**, and it gives a number anyway.

Experiment, method stated: run once, outside the suite, because the suite never
executes undefined behaviour on purpose. gcc 11 and clang 14, at `-O0` and `-O2`,
with the value read from a `volatile` so it could not be folded:

| value | `static_cast<int>` gave |
| --- | --- |
| 1e10 | -2147483648 |
| -1e10 | -2147483648 |
| nan | **-2147483648** |
| inf | -2147483648 |
| 2147483648.0 | -2147483648 |

Not a number, infinity, and a number one too large are all the **most negative
int**. That is also a perfectly good reading, so afterwards there is nothing to
tell them apart with. A sensor drops out, becomes `nan`, and becomes a plausible
motor command.

The sanitizers disagree about it. Measured: gcc's `-fsanitize=undefined`
reported nothing, and clang's reported it. The separate
`-fsanitize=float-cast-overflow` reported it on both. A sanitizer lane that is
gcc alone will not see this unless it asks for the option by name, which is
lesson 04-07's point about what each sanitizer will not tell you.

So the question is asked **before** the conversion. `fits_int` says whether a
`double` is a number an `int` can hold. `nan` fails both comparisons against
the limits, so it is refused for the same reason as everything else that is not
such a number, with no special case.

`E-NUM-0027`.

### Wide into narrow wraps, and quietly

```
   seconds   microseconds   static_cast<int>   to_int
        60       60000000           60000000       ok
      2000     2000000000         2000000000       ok
      2147     2147000000         2147000000       ok
      2148     2148000000        -2146967296  refused
      3000     3000000000        -1294967296  refused
```

A microsecond clock is comfortable in a `long long`. Held in an `int` it works
for 2147 seconds and is negative at 2148, which is **35.8 minutes**. A robot that
has been up for 36 minutes has a negative time, and the first thing to notice is
a timeout that fires at once.

Converting down is defined by the implementation, not the standard, so the four
toolchains this curriculum claims all wrap. That is why it is a believable
negative number and not a crash.

### A float runs out of whole numbers

```
  16777217 as a float: 16777216
  100000001 as a float: 100000000

  of 100 integers starting at 16777216, a float cannot hold  50
  of 65536 integers across the whole int range, a double cannot hold  0
```

A `float` holds every whole number exactly up to 16777216, which is two to the
24th, and not above it. A `double` holds every 32 bit integer, and none of 65536
samples across the whole range was lost.

An encoder count stored as a `float` is exact for a while and then stops being
so, by one and then by two, with nothing to say when.

`E-NUM-0028`.

### Braces are the one place the language checks

```cpp
double d = 3.7;
int x{d};          // refused by every compiler
```

`checks/a_narrowing_brace.cpp` is that refusal, compiled by the build and
expected to fail. Brace initialisation is the only conversion C++ checks for
loss, so it is the cheapest guard there is: prefer it, and when the loss is
intended, use the cast that says so.

One note on which cast. The old spelling, `(int)d`, tries several kinds of cast
in turn, including ones that reinterpret memory, and the compiler picks. A
`static_cast` is the narrow one and refuses what it cannot do, which is why this
curriculum writes it out.

## Build It

Four pieces, all returning a value the caller has to look at.

`floor_to_int` and `round_to_int` take the whole number with `std::floor` or
`std::round`, ask `fits_int` about **that**, and only then cast. A double that
does not fit must never reach `static_cast<int>`.

`to_int` takes a `long long` and compares it against the smallest and largest
`int` **as `long long`**, so the comparison cannot overflow.

`survives_float` casts an `int` to `float` and back to `double` and compares
with the original as a double. A double holds every `int` exactly, so the
comparison itself loses nothing.

The stubs refuse everything and return `false`. The suite checks that valid
inputs are accepted as well as invalid ones refused, because a function that
refuses everything would otherwise pass the second half.

## Use It

```cpp
#include <rc/core/narrow.hpp>

const auto cell = rc::core::floor_to_int(position / cell_size);
if (!cell.ok) { /* off the map, or not a number */ }

const auto micros = rc::core::to_int(elapsed_micros);
if (!micros.ok) { /* keep it as a long long */ }
```

`ok` is a separate field because no `int` can stand for "it did not fit". Every
one of them is a reading some sensor could produce, and `-2147483648` is
exactly what the raw cast gives for `nan`, so a sentinel would be the same
silent wrong answer under another name.

Two limits, stated plainly.

**These check one conversion.** They do not follow a number through a
computation, and a value that is fine as a `double` and wrong as the `float` it
was stored in before this call is out of their reach.

**`float` to `int` and `unsigned` conversions are not covered.** Unsigned was
lesson 01-07, and the float case follows the same rules at a smaller scale.

## What Breaks First

`E-NUM-0026` is the cast that was not rounding, and the cell at the origin that
was twice the size of the others.

`E-NUM-0027` is the number that was not a number and became a plausible one,
with the table of what the raw cast gave and which sanitizers see it.

`E-NUM-0028` is the value that did not fit where it was put: the clock that goes
negative after 36 minutes, the float that stops holding a count, and the one
place the compiler checks.

## Ship It

`rc/core/narrow.hpp`, in `namespace rc::core`.

From here on, a `double` becomes an `int` through `floor_to_int` or
`round_to_int`, and a clock stays a `long long` unless `to_int` says it fits.
