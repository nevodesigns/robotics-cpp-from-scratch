id: E-SENSE-0028
title: The status byte ignored
match: a frame that is short, not ready or overrun says so, and is not read as data
platforms: linux, windows
teaches: 15-12-the-register-that-packs-three-things
---

## Symptom

The same reading arrives twice, or a lost frame is never noticed, and a derived speed has a gap in it.

## Cause

The status byte says whether the data is new and whether any was lost. A driver that reads the bytes without looking treats a stale frame as a fresh one and drops the news that a frame went missing. Three answers differ: too short (untouched), not ready (untouched) and overrun (good data, 0.1953 g in the test, with the loss to be recorded).

## Fix

Return a status with the sample and handle each.

```cpp
switch (decode_frame(bytes, count, sample)) {
  case DecodeStatus::Overrun: use(sample); note_lost_frame(); break;
  default: break;
}
```
