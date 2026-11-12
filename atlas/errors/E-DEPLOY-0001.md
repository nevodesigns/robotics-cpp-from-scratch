id: E-DEPLOY-0001
title: A struct sent as it lies in memory to a machine that lays it out differently
match: the same struct is a different size on each target, because of long, pointers and the alignment of an eight byte field
match: a struct can be sent as it lies in memory only if it has one size on every target and no field whose width changes
platforms: linux, windows
teaches: 20-01-the-target-is-not-the-host
---

## Symptom

The program works between two laptops and the robot reads nonsense, with the same header on both sides and no error anywhere.

## Cause

A struct of a char, an int64, a pointer, a long and a double was 40 bytes on x86_64 Linux, aarch64 and mingw, 32 on 32 bit ARM and 28 on i686. A struct of a char and an int64 was 16 bytes and 12 bytes, with nothing in it but fixed width types.

## Fix

Write fields in a defined order and width, and static_assert the size.

```cpp
static_assert(sizeof(Message) == 12);
```
