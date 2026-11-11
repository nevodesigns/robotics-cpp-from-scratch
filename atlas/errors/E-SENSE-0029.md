id: E-SENSE-0029
title: A frame trusted without a check
match: the CRC-8 of the standard test string is 0xF4, and a frame checks only if its last byte is the CRC
match: a CRC catches every single bit flip and every swap of two bytes, which a plain sum does not
platforms: linux, windows
teaches: 15-13-the-read-that-fails
---

## Symptom

A reading is occasionally far from its neighbours for one sample, with no error reported anywhere.

## Cause

Without a check, a flipped bit is a different number. A plain sum is a check that cannot see order. Measured on a five byte frame: the CRC missed 0 of 40 single bit flips and 0 of 6 swaps of two bytes, and a plain sum missed all 6 swaps. The CRC of 123456789 is 0xF4.

## Fix

End the frame with a CRC-8 and decode only a frame that passes.

```cpp
if (frame_ok(frame, size)) decode(frame);
```
