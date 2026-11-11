# The Field That Was Quietly Zero: Reading JSON Without Letting a Missing Value, a String or a Big Number Pass for a Good One

> A configuration with one misspelt key loads without a word, and the robot runs with a speed of zero. Qt's JSON reader
> has a way of answering that makes every kind of problem look the same.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 09-06, 03-02

## The Problem

Telemetry and configuration are JSON in most robotics software, and Qt reads it with `QJsonDocument`. The convenient way to get a number out is `value.toDouble(default)`: it never fails and never throws.

That is the problem. A speed that is missing, a speed written as `"1.5"` in quotes, a speed that is `null` and a speed that really is the default all come out as the default, and the program cannot tell which happened.

## The Concept

### One answer for five problems

Read from the same object with `toDouble(-1)`:

| the field is | answer |
|---|---|
| missing | -1 |
| the string "1.5" | -1 |
| null | -1 |
| true | -1 |
| really -1 | -1 |

A number with a fraction read with `toInt(-1)` gives -1 as well. See `E-QT-0033`.

### Look before converting

`read_number` takes the value with `value(key)`, which is `Undefined` for a missing key, and asks what it is before converting it:

- undefined: **missing**;
- not a number: **wrong type** (a string, null and true all are);
- outside the limits the caller gave, or not a number: **out of range**.

A real -1 is read as one. 1.5 against a limit of 1 is out of range, 1e300 against 1000 is too, and the limits themselves are allowed. The caller gets a reason it can put in a log: a typo in a key is now a message.

### An identifier that is a whole number

A double holds whole numbers exactly only up to 2^53, **9007199254740992**. A reader that turns every number into a double hands back the nearest one that fits. The identifier **9007199254740993** came back as 9007199254740992 through `toDouble`, a different identifier, and exactly through `toInteger`.

`read_id` keeps whole numbers whole. 3.7 was refused as **not whole**, and so was 12345678901234567890, which is too big for 64 bits and arrived as a double; a string "7" was the wrong type. Better still, send large identifiers as strings. See `E-QT-0034`.

### A document that says why

`parse_object` keeps the parser's own message and adds the check it does not make. On Qt 6.2 a trailing comma gave "object is missing after a comma", a number of 1e999 gave "illegal number", and `[1,2,3]`, which is valid JSON, was refused as "not an object". See `E-QT-0035`.

## Build It

`read_number` and `read_id` in `exercise/solution.hpp`. `FieldError`, `describe` and `parse_object` are given.

```
rcpp verify 09-08
```

## Use It

`rc/qt/json_fields.hpp` joins `rc::qt`.

```cpp
const NumberField speed = read_number(config, "speed", 0.0, 2.0);
if (speed.error != FieldError::None) { /* "speed: " + describe(speed.error) */ }
```

**What this does not do.** It reads numbers and whole identifiers, not strings, arrays or nested objects, and a real reader needs those too. It does not say which key is unknown: a typo in an optional key still goes unnoticed, as in lesson 04-12, where the known names were listed and the rest suggested. The messages are Qt's, and they
change between versions, so the tests check that there is one and not what it says. And JSON itself has no way to say a number is an integer: `1.0` and `1` are the same to a reader that does not look.

## What Breaks First

- **A default for every problem.** The same answer for five causes. See `E-QT-0033`.
- **An identifier read as a double.** The last digit changes. See `E-QT-0034`.
- **A parse that says only that it failed.** Half an hour to find a comma. See `E-QT-0035`.

## Ship It

`rc/qt/json_fields.hpp` holds `FieldError`, `describe`, `NumberField`, `read_number`, `IdField`, `read_id` and `parse_object`.

Lesson 09-06 read a number from text with a locale named. This reads it from a document that has a type for it, and asks what the type is.
