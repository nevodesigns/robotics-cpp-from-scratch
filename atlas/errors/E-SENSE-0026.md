id: E-SENSE-0026
title: A signed register read as unsigned
match: a twelve bit number with its top bit set is negative, and sign extension is what says so
platforms: linux, windows
teaches: 15-12-the-register-that-packs-three-things
---

## Symptom

The reading is right when the joint is tilted one way and jumps to near full scale when it is tilted just the other way.

## Cause

A twelve bit two's complement number is negative when its top bit is set. Without sign extension that bit is just 2048. Measured:

| raw 12 bit | unsigned | signed |
|---|---|---|
| 0x7FF | 2047 | 2047 |
| 0x800 | 2048 | -2048 |
| 0xFFF | 4095 | -1 |

## Fix

Sign extend from the register's width.

```cpp
const int count = sign_extend(word >> 4, 12);
```
