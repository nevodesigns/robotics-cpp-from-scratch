id: E-SENSE-0027
title: A reading wider or narrower than its register
match: a frame is decoded to g, with negative readings negative
match: every one of the 4096 counts on every range decodes to the count times the size of a count
platforms: linux, windows
teaches: 15-12-the-register-that-packs-three-things
---

## Symptom

Every reading is sixteen times too large, or the lowest four bits of the next field are mixed into this one.

## Cause

The datasheet's twelve bits sit in the top of sixteen, so the low four bits are not part of the reading. A frame of 01 40 00 E0 00 80 00 at plus or minus 2 g is x 1.000, y -0.500 and z -2.000 g only after shifting each word right by four. Across all 16384 frames the shifted decode had 0 axes wrong.

## Fix

Shift and mask first, then sign extend, then scale by the range's size of a count.

```cpp
const int count = sign_extend(word >> 4, 12);
const double g = count * range_in_g(code) / 2048.0;
```
