id: E-BUILD-0013
title: A cache key that leaves out the flags
match: the same source with different flags, or a different name, is a different build
match: expected this to be true: a != build_key
platforms: linux, windows
teaches: 04-11-the-build-that-rebuilt-nothing-or-everything
---

## Symptom

A release build is made after a debug build and contains the debug code, or a build with the assertions on is quietly reused where they were meant to be off. The source did not change, so the cache said it was fine.

## Cause

An object depends on more than the text of the source: on the flags, the compiler and the file's own name (it can appear in the object). A key made from the text alone gives the same answer for builds that must differ.

The suite checked that the key changes with the flags (`-O2` against `-O0` and against `-O2 -DNDEBUG`), with the name and with the text, and that swapping what two fields hold does not give the same key. Over 300 random builds of one file, 4 versions and 2 sets of flags, the cache compiled **8** times, hit **292**, and served **0** stale objects.

## Fix

Put everything the output depends on into the key:

```cpp
scramble(a ^ (b * 0x9E3779B97F4A7C15ULL) ^ (c * 0xC2B2AE3D27D4EB4FULL));
```

The multiplications stop the three parts cancelling when two of them are equal.
