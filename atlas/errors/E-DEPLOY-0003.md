id: E-DEPLOY-0003
title: Bytes in the other machine's order
match: a number is laid out in the byte order of the machine, and the same four bytes mean different numbers on the other
platforms: linux, windows
teaches: 20-01-the-target-is-not-the-host
---

## Symptom

A value arrives as a huge number, or as a small one with its bytes swapped, from a controller that is not like the laptop.

## Cause

0x01020304 was 4 3 2 1 in memory little endian and 1 2 3 4 big endian. The big endian bytes read as little endian gave 67305985, which is 0x04030201.

## Fix

Pick one order for the wire and convert at the edges.

```cpp
const auto bytes = encode_u32(value, Endian::big);
```
