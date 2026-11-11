id: E-SENSE-0030
title: A retry that is not counted
match: a bounded read gives up after its attempts and says whether it timed out or got a bad frame
platforms: linux, windows
teaches: 15-13-the-read-that-fails
---

## Symptom

The driver never reports an error and the latency has crept up, because every read is taking several tries.

## Cause

A retry that quietly succeeds on the third attempt hides a bus that is failing. A dead bus used 4 attempts and 4 timeouts, a noisy bus 3 attempts and 3 bad frames and a good bus 1 attempt. Timeouts and corrupt frames are different faults and point at different causes.

## Fix

Keep the counts and surface them.

```cpp
const ReadOutcome out = read_reliably(read, 4, frame, size);
if (!out.ok) report(out.timeouts, out.corrupt);
```
