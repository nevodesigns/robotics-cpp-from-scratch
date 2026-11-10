id: E-BUILD-0010
title: A build line that is read back as something it is not
match: a line that is not exactly that format is refused, and leaves the output alone
match: expected count
platforms: linux, windows
teaches: 04-10-what-binary-is-this
---

## Symptom

A log processor reads the build line from a report. A field is missing, or a new compiler appears, or the line has two spaces in it, and the processor reads the wrong column as the sanitizer or gets a version of zero, and files the report under the wrong build.

## Cause

Parsing by position without checking each piece accepts a line that only looks like the format. A report is written by something that changes: a new compiler, a different order, a hand edit.

The suite wrote five builds (gcc, clang, msvc, a 32 bit big endian build, and a C++23 build) and read them back: **5 of 5** unchanged. It then gave **12** malformed lines: empty, a word missing, a word too many, a double space, a compiler with no version, an unknown compiler, `c17` for `c++17`, `fast` for release or debug, an unknown sanitizer, `64` without `bit`, `middle` for the byte order, and a leading space. All 12 were refused and the output was left alone.

## Fix

Check the number of pieces and then each piece against what it may be, and write to the output only when all are right:

```cpp
if (p.size() != 6) return false;
if (p[3] != "none" && p[3] != "address" && p[3] != "thread") return false;
```
