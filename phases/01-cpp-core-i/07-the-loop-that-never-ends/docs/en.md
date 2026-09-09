# The Loop That Never Ends: Sizes Are Unsigned, and Arithmetic Does Not Care

> On an empty vector, `size() - 1` is not -1. It is 18446744073709551615, and
> nothing anywhere warns you.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 01-04

## The Problem

You have a vector of sensor readings and you want to walk it backwards, newest
first. You write the obvious loop:

```cpp
for (std::size_t i = readings.size() - 1; i >= 0; --i) {
    use(readings[i]);
}
```

It hangs. Or it runs for a very long time and then crashes somewhere else
entirely. It works on your test data and fails on the robot, or it works on the
robot and fails the one time the sensor sends nothing.

Nothing in that line is a typo. Every part of it is doing exactly what C++ says
it should.

## The Concept

### Sizes cannot be negative, so they wrap instead

`readings.size()` is a `std::size_t`, which is unsigned. Unsigned means it
cannot represent a negative number, and C++ does not respond to that by giving
you an error. It responds by wrapping around.

| | value |
|---|---|
| `size()` on an empty vector | 0 |
| `size() - 1` | 18446744073709551615 |
| `last_index(size())` | -1 |

That middle row is not a large number by accident. It is exactly the largest
number a `std::size_t` can hold, because 0 minus 1 in unsigned arithmetic wraps
to the top.

This matters more than it looks, because **it is not undefined behaviour**. The
program is not doing anything the standard forbids, so no sanitiser reports it,
no assertion fires, and the value that comes out is a perfectly good number. It
is just not the number you meant.

### That is why the loop never ends

Two separate failures live in that one line.

The first is the starting value: on an empty container the loop begins by
indexing 18446744073709551615.

The second is the condition. `i >= 0` where `i` is unsigned **cannot become
false**. Nothing unsigned is ever less than zero. So when `i` reaches 0 and
`--i` runs, it does not become -1 and stop, it wraps to the top and carries on.

Measured over a three element vector, with the loop capped so it could be
reported at all:

```
    unsigned index, capped at 10:  10 steps
    signed index:              3 steps
    signed index, empty:       0 steps
```

Three readings, and the unsigned version is still going after ten passes.

### The comparison that flips

The same conversion catches you somewhere quite different. A function returns -1
for "not found", and you check it against a size:

| | value |
|---|---|
| `(std::size_t)(-1)` | 18446744073709551615 |
| `(std::size_t)(-1) < 3` | false |
| `-1 < ssize(3)` | true |

The answer flips. When you mix a signed and an unsigned value in a comparison,
the signed one is converted to unsigned, and -1 becomes the largest value there
is rather than the smallest.

### What the compiler will and will not tell you

This is the part worth remembering, because it decides how much you can lean on
the toolchain. Measured under `-Wall -Wextra`:

| | gcc | clang | warning |
|---|---|---|---|
| `int i < v.size()` | refuses | refuses | `-Wsign-compare` |
| `-1 < v.size()` | refuses | refuses | `-Wsign-compare` |
| the same, with the -1 `const` | refuses | **accepts** | `-Wsign-compare` |
| unsigned countdown, `i >= 0` | refuses | **accepts** | `-Wtype-limits` |
| `v[v.size() - 1]` | accepts | accepts | none |
| `int n = v.size()` | accepts | accepts | none |

Three things follow.

**The comparisons are covered.** Both compilers object to mixing signed and
unsigned in a comparison, and this lesson ships two of those in `checks/` as
code that must not compile.

**The coverage is uneven, and it moves.** gcc reports the countdown condition
and clang does not. Worse, look at rows two and three: writing the sentinel as
`const int missing = -1` instead of `int missing = -1` makes clang fold it to a
known value and stop warning, while gcc still refuses. Adding `const` to a
variable removed a diagnostic. That was found by this lesson's own
must-not-compile check failing on the clang lane and passing on gcc.

If you build on one compiler you are relying on a net with a known hole in it,
and that is a large part of why this curriculum builds every lesson on gcc,
clang and MSVC on every push.

**The arithmetic is not covered at all.** `size() - 1` produces no diagnostic
from anything, and it is the dangerous one. So is `int n = v.size()`, which is a
silent narrowing that is harmless for a hundred readings and not harmless for a
point cloud.

### The repair

Four small functions, and the value is not in the code, it is in never writing
`size() - 1` again.

```cpp
std::ptrdiff_t ssize(std::size_t size);                          // signed count
std::ptrdiff_t last_index(std::size_t size);                     // -1 when empty
bool is_valid_index(std::ptrdiff_t index, std::size_t size);     // both ends
bool fits_in_int(std::size_t size);                              // before narrowing
```

`last_index` is the one that does the work. It returns -1 for an empty
container, which is a number a signed loop counter can compare against and stop
on:

```cpp
for (std::ptrdiff_t i = last_index(readings.size()); i >= 0; --i)
```

Three passes over three readings, zero over none.

C++20 has `std::ssize` in the standard library. This is the same thing for the
C++17 baseline, named the same on purpose, so the day the curriculum moves to
C++20 it is a one line change rather than a hunt.

## Build It

Implement `ssize`, `last_index`, `is_valid_index` and `fits_in_int` in
`exercise/solution.hpp`.

```
rcpp verify 01-07
```

The suite prints what an empty size does to subtraction, runs the broken
countdown against a cap to show it does not stop, watches the comparison flip,
and walks a real vector in both directions. Two snippets in `checks/` are
compiled and expected to fail.

## Use It

**Never write `size() - 1`.** It is correct only when you already know the
container is not empty, which is exactly when you did not need it.

**If you want the last element, say so.** `readings.back()` does no arithmetic,
so it cannot have an arithmetic bug. Check `empty()` first.

**Prefer no index at all.** A range based for, or a reverse iterator, removes
the whole category:

```cpp
for (auto it = readings.rbegin(); it != readings.rend(); ++it)
```

**Do not silence a sign warning with a cast.** `(std::size_t)found < size()`
makes the message go away and keeps the bug.

**Build on more than one compiler.** The table above is the argument, and it
cost nothing to find out.

## What Breaks First

- **A countdown that never reaches the end.** See `E-CPP-0028`.
- **A negative sentinel that stops being negative.** See `E-CPP-0029`.
- **A size put into an int, quietly.** See `E-CPP-0030`.

## Ship It

`ssize`, `last_index`, `is_valid_index` and `fits_in_int` join `rc::core`.
Every loop in the rest of this curriculum that walks a container by index uses
them, which is why none of them has this bug in it.
