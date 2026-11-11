# The Setting That Was Converted: What QVariant Does to 3.7, to 3e9 and to the Word "no"

> A QVariant will turn almost anything into almost anything and tell you it worked. A limit becomes another limit,
> a count becomes a negative number, and the word "no" turns out to mean yes.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 09-08, 00-10

## The Problem

Qt keeps settings, property values and the contents of a JSON file in a `QVariant`, a box that holds a value of any type. Asking it for an `int` or a `bool` is one call, and the call always gives an answer.

Lesson 09-08 found that reading JSON with a default makes five problems look alike. This is the same trouble one layer in: the conversions themselves change the value, and the `ok` that comes with them does not mean what it appears to.

## The Concept

### Numbers

On Qt 6.2, with `ok` true each time:

| call | result |
|---|---|
| `QVariant(3.7).toInt()` | **4** |
| `QVariant(-3.7).toInt()` | **-4** |
| `QVariant(3e9).toInt()` | **-1294967296** |
| `QVariant(-1).toUInt()` | **4294967295** |

A double is rounded and a number that is too big wraps, and `ok` says only that a conversion was done. See `E-QT-0039`.

`setting_int` takes a whole number only if it is one and fits an int. It read 42, -7, the double 5.0, the string "12", the largest int and a `qlonglong` of 1000 exactly. It refused all **13** of: 3.7, -0.5, 3e9, -3e9, a `qlonglong` and a `quint64` too big for an int, the strings "3.7", "1e3", "12abc" and "", true, an empty variant, and not a number. See `E-QT-0041`.

### Words

`QVariant::toBool` on a string is false for the empty string, "0" and "false", and true for anything else:

| text | `toBool()` |
|---|---|
| false, 0, (empty) | false |
| true, 1, yes | true |
| **no** | **true** |
| **off** | **true** |
| **abc** | **true** |
| **2** | **true** |

`QVariant(0.1).toBool()` is false, since a double is cut to a whole number first, and not a number is true. A setting `safety = off` leaves the safety on. See `E-QT-0040`.

`setting_bool` reads true, yes, on and 1, and false, no, off and 0, in any case with spaces trimmed: **13** of 13 spellings were read correctly. A number must be exactly 0 or 1. It refused **9** of 9 that are neither: "abc", "2", the empty string, "maybe", the numbers 2, 0.5 and -1, not a number, and an empty variant.

### A double, in the plain locale

`setting_double` takes a number, or text read in the plain locale (lesson 09-06), and refuses a comma, `abc`, not a number, infinity, true and an empty variant. `"1e3"` is 1000 here, which `toInt` refused: asking for a double is the way to say it.

## Build It

`setting_int`, `setting_bool` and `setting_double` in `exercise/solution.hpp`.

```
rcpp verify 09-10
```

## Use It

`rc/qt/settings_convert.hpp` joins `rc::qt`.

```cpp
bool ok = false;
const int count = setting_int(settings.value("count"), &ok);
if (!ok) { /* say that "count" is not a whole number, and use the documented default */ }
```

**What this does not do.** It does not choose a default: a refused setting needs one from somewhere, and a missing setting needs the same decision. It reads the handful of types a settings file holds, and a `QVariant` holding a list, a map or a custom type is refused. The behaviour of `QVariant` is Qt 6.2's as measured, and newer Qt versions may convert some of these differently, which is why the tests on `QVariant` itself are
here as measurements and the continuous integration runs on the others. And it does not tell you which setting was wrong: the caller knows the key.

## What Breaks First

- **A number converted with `ok` true.** 3.7 is 4. See `E-QT-0039`.
- **A word taken as true.** "no" and "off" are. See `E-QT-0040`.
- **A number as text that is not digits.** Zero. See `E-QT-0041`.

## Ship It

`rc/qt/settings_convert.hpp` holds `setting_int`, `setting_bool` and `setting_double`.

Lesson 00-10 asked what a person typed and refused what it could not read. This asks the same of a box that thinks it can read anything.
