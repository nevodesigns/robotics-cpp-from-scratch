id: E-SAFE-0034
title: A black box that calls itself damaged when it has only wrapped
match: when the ring is full the oldest event goes, and what is left still verifies because the chain starts from the last one dropped
match: events are kept in order with a sequence number and a chain, and an untouched log verifies
platforms: linux, windows
teaches: 19-12-what-happened-before-the-stop
---

## Symptom

After the first night the log fails its own check from the first entry, and the check is switched off.

## Cause

Twenty events into a ring of 8 left sequence 12 to 19. Verified from the kept chain value: yes. Started from zero: no, the first event was called altered. After 100 events, with 92 dropped, it still verified.

## Fix

Keep the chain value of the last event overwritten and start from it.

```cpp
base_chain_ = events_[head_].chain;
```
