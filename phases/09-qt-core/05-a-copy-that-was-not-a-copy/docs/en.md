# A Copy That Was Not a Copy: Implicit Sharing, and the Innocent Read That Makes a Real One

> Qt copies a buffer by sharing it until one of the copies is written to. A read that looks innocent can be that write,
> and a loop makes it for every frame.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 09-01, 02-05

## The Problem

A telemetry handler gets a frame of 4096 bytes through a signal, loops over the bytes to add them up, and passes the frame on. It is slower than it should be, and the profile shows `memcpy` and `malloc` inside Qt. Nothing in the handler writes to the frame.

Lesson 02-05 explained copying, and lesson 02-15 counted what a `std::vector` copies. Qt's strings and arrays take another route to the same cost: they do not copy until they have to, and what makes them have to is not what you would guess.

## The Concept

### Implicit sharing

A copy of a `QByteArray` copies a pointer and a count. Two arrays then point to the same bytes. The bytes are copied only when one of them is **written to**, and that one then takes its own: it **detaches**.

```cpp
QByteArray b = a;        // shares: the same bytes
b[0] = 99;               // a write: b takes its own copy, a is untouched
```

The test checked it: after `b = a` the two had the same bytes, and after writing `b[0]` they did not, with `a[0]` still 0 and `b[0]` 99. See `E-QT-0024`.

### What counts as writing

Qt decides by whether the object is const, not by what the code does. On an array that is not const, `operator[]` and `data()` are the versions that can write, so they detach before returning. Measured on a copy of a 1000 byte array, while only looking:

| call | copied the bytes |
|---|---|
| `b[0]` | yes |
| `b.at(0)` | no |
| `b.data()` | yes |
| `b.constData()` | no |
| `b.size()` | no |

### The loop that copies every frame

A range-for calls `begin()`, and on a non-const array that is the writing version. 1000 frames of 4096 bytes, each a copy of one shared array, read by a range-for: **1000** copied through a non-const array and **0** through `std::as_const`, which gives the loop a const view:

```cpp
for (char c : std::as_const(frame)) total += static_cast<unsigned char>(c);
```

See `E-QT-0025`.

### A function that really writes

`with_marker` takes a frame by value, sets the first byte and returns it. It has to copy, and the aim is exactly once and not into the caller's own bytes. The result's first byte was **77** and the caller's still **0**, and the caller's pointer did not change. See `E-QT-0026`.

## Build It

`sum_const`, `sum_range_for` and `with_marker` in `exercise/solution.hpp`.

```
rcpp verify 09-05
```

## Use It

`rc/qt/shared_data.hpp` joins `rc::qt`.

```cpp
int total = rc::qt::sum_const(frame);        // a const reference and at(): nothing can copy
```

**What this does not do.** It is the behaviour of Qt 6.2 as measured here, and other versions may differ in detail; the continuous integration runs on newer ones, which is how a difference would show. It says nothing of `QString`,
`QList` or `QMap`, which share in the same way and have the same const and non-const pairs. A copy made by detaching is the same cost as a `std::vector` copy, so the lesson is only about not paying it by accident. And sharing across threads is safe for the count and not for writes: lesson 09-03 is the place for that.

## What Breaks First

- **A read through a non-const array.** It copies the bytes. See `E-QT-0024`.
- **A range-for over a non-const container.** It copies once for every frame. See `E-QT-0025`.
- **Writing through a reference.** The caller's bytes change. See `E-QT-0026`.

## Ship It

`rc/qt/shared_data.hpp` holds `sum_const`, `sum_range_for` and `with_marker`.

Lesson 09-03 sent a value across a thread boundary by copying it. This is why that copy is cheap, and the one way it stops being cheap.
