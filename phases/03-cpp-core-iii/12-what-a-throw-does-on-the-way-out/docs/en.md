# What a Throw Does on the Way Out: Catching by Reference, Rethrowing, and the Destructors That Run

> Lesson 03-02 said when an error should be an exception. This one says what an exception does once it is
> thrown, and the three small habits that keep it from losing what made it useful.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 01-05, 03-01, 03-02

## The Problem

Lesson 03-02 argued that most errors are not exceptional and should be values, and that a constructor which cannot build a valid object should throw. It did not show what a throw does. A program that is meant to
use exceptions, or that calls a library which does, needs three habits, and each is a one word difference that nothing warns about.

## The Concept

### Different errors, different types

`parse_positive` turns text into a whole number above zero, and throws one of two types:

- `std::invalid_argument`: the text is not a number, such as `12abc`.
- `std::out_of_range`: it is a number and is no use, such as `0`, `-3` or `99999999999999999999`.

A caller can handle the two differently. The message of each carries the text that caused it.

### Catch by reference

```cpp
catch (const std::invalid_argument& e) { ... }
catch (const std::out_of_range& e) { ... }
```

Most specific first, and by const reference. Catching by value makes a new `std::exception` by copying the base part of what was thrown, and everything that made it specific is cut off.

Thrown `invalid_argument("the specific message")`: caught by value gave **"std::exception"**, and by reference gave **"the specific message"**. See `E-CPP-0085`.

### Rethrow with nothing after it

A function can catch an error, note something and let it carry on. `throw;` with nothing after it rethrows the object that was caught. `throw e;` throws a copy of the variable, made from its declared type.

| written | what the caller sees |
|---|---|
| `throw e;` | cut down to exception |
| `throw;` | still invalid_argument |

`parse_all` notes which item failed and then uses `throw;`, so the caller still knows whether it was a bad number or a bad range. See `E-CPP-0086`.

### What runs on the way out

A throw leaves the function at once. Nothing after the throwing line runs. What does run is the destructor of every object alive in each scope being left, in reverse order of construction, and before the handler starts:

```
make A, make B, make C, drop C, drop B, drop A, handler
```

That is why cleanup belongs in a destructor (lesson 02-03). Cleanup written after the call that throws is skipped. See `E-CPP-0087`.

## Build It

`parse_positive`, `try_parse_positive` and `parse_all` in `exercise/solution.hpp`. The comments give each handler and say which order they go in.

```
rcpp verify 03-12
```

## Use It

`rc/core/checked_int.hpp` joins `rc::core`.

```cpp
int n = 0;
std::string why;
if (!rc::core::try_parse_positive(text, &n, &why)) { std::cerr << why << "\n"; }
```

**What this does not do.** It parses decimal whole numbers only, and it is not a replacement for the checked reader of lesson 00-10, which is for a person at a keyboard and reports without throwing. It says nothing about how much a throw costs, which is large next to a
return and is why lesson 03-02 keeps exceptions for rare events. A throw from a destructor during unwinding ends the program, so destructors must not throw. And a library built without exceptions, which many robotics systems are, turns a throw into
an immediate stop.

## What Breaks First

- **Catching by value.** The message is cut off. See `E-CPP-0085`.
- **`throw e;`.** The type is cut down. See `E-CPP-0086`.
- **Cleanup after the throwing call.** Skipped. See `E-CPP-0087`.

## Ship It

`rc/core/checked_int.hpp` holds `parse_positive`, `try_parse_positive` and `parse_all`.

This is the other half of lesson 03-02: when the error is exceptional, this is how to throw it and catch it without losing anything.
