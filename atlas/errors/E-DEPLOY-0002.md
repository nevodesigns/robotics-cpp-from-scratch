id: E-DEPLOY-0002
title: A target triple that was read by guessing
match: a triple is read into the word sizes and byte order of the machine, in either of its shapes
match: a triple that cannot be read is invalid, not a guess
platforms: linux, windows
teaches: 20-01-the-target-is-not-the-host
---

## Symptom

The build succeeds for the wrong machine, or a long that must be 64 bits is 32 only on the Windows build.

## Cause

x86_64-w64-mingw32 and x86_64-pc-windows-msvc read as 64 bit pointers with a long of 32 bits. Nine malformed triples (empty, two parts, an empty part, an unknown architecture or system, five parts) were all invalid, with a pointer width of 0.

## Fix

Refuse what cannot be read, and use fixed width types.

```cpp
if (!parse_triple(text).valid) return 2;
```
