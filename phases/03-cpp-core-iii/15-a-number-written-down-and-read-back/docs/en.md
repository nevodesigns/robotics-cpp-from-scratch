# A Number Written Down and Read Back: Why a Log or a Config Loses Digits, and the Shortest Text That Does Not

> A number is written to a log, a file or a message, and read back later. The ordinary ways of writing it
> change it, and which way depends on how big it is.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 00-04, 01-05, 03-02

## The Problem

A run is logged so that it can be replayed, or a calibration is saved to a file and loaded at the next start. The numbers go out as text and come back in. If they come back different, the replay does not replay, and a calibration that was good is slightly wrong in a way no
one sees.

`std::cout << x` is the first thing everyone writes, and `std::to_string(x)` the second. Neither gives the number back.

## The Concept

### Six digits is not the number

A stream writes six significant digits. A `double` carries about sixteen, and up to seventeen are needed to tell it from its neighbour. Of 20000 readings of mixed size written as text and read back, to see whether each was exactly the same number:

| way of writing | read back exactly |
|---|---|
| stream, six digits | **0** |
| `std::to_string` | **5627** |
| `shortest_text` | **20000** |

`std::to_string` writes large numbers in full, which is why some survive. See `E-NUM-0034`.

The sum `0.1 + 0.2` and the number `0.3` are different doubles. `shortest_text` writes them as `0.30000000000000004` and `0.3`. Six digits write both as `0.3`, so a log cannot say they differ.

### Small numbers vanish

`std::to_string` always writes six places after the point. A reading of 3.5e-9 became **"0.000000"**, where `shortest_text` gave `3.5e-09`, which reads back exactly. A time step or a gain that small is read back as zero and then divided by. See `E-NUM-0035`.

### The shortest text that reads back

Writing seventeen digits always is exact and ugly. `shortest_text` tries one significant digit, then two, up to seventeen, and stops at the first text that `strtod` reads back as the very same double:

```cpp
std::ostringstream out;
out << std::setprecision(digits) << value;
if (std::strtod(out.str().c_str(), nullptr) == value) break;
```

For readings to two decimal places the mean length was 5.8 characters, against 15.7 at a fixed seventeen digits. A whole number below 1e15 is written as itself, with `std::fixed` and a precision of 0: without that rule one significant digit writes 100 as `1e+02`.

Not a number and infinity have no digits, and are written as `nan`, `inf` and `-inf`.

### Reading it back safely

`parse_number` uses `strtod` and then looks at what it did not use. It refuses ten bad texts: empty, spaces only, `1.5x`, `1,5`, `two`, `1 2`, `nan`, `inf`, `-inf` and `1e999`. `nan` and `inf` are words `strtod` accepts, and `1e999` becomes infinity, and none of them is a number a
configuration means. A refusal leaves the output as it was. See `E-NUM-0036`.

## Build It

`shortest_text` and `parse_number` in `exercise/solution.hpp`. `default_text`, the stream version, is given for comparison.

```
rcpp verify 03-15
```

## Use It

`rc/core/number_text.hpp` joins `rc::core`.

```cpp
log << rc::core::shortest_text(value) << "\n";
...
double v;
if (!rc::core::parse_number(field, &v)) { /* say which field */ }
```

**What this does not do.** It is for `double`. A `float` needs at most nine digits and the search would stop sooner for it, but the text read back as a `double` is not the same number as the float. It does not deal with the locale: a program run with a locale that uses a comma as the decimal point may
write `1,5`, which `parse_number` refuses. It is not the fastest way to write a number, since each digit count writes and reads once, and a log of millions of values may want a method that does it in one step. And exact round trip is a property of the number, not of the sensor: a reading that was noise is still noise.

## What Breaks First

- **A plain stream.** Six digits. See `E-NUM-0034`.
- **`std::to_string`.** Small numbers become zero. See `E-NUM-0035`.
- **A parse that ignores the rest.** `1.5x` and `nan` get through. See `E-NUM-0036`.

## Ship It

`rc/core/number_text.hpp` holds `default_text`, `shortest_text` and `parse_number`.

Lesson 00-04 found that numbers lie inside a program. This is how they lie on the way out of it.
