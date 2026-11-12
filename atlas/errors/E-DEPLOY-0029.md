id: E-DEPLOY-0029
title: A timeout on the wall clock
match: the wall clock jumps by decades when it is set and by fractions of a second when it is slewed, and the monotonic clock never jumps
match: a timeout measured in wall time fires when the clock is set, and one measured in time since boot does not notice
platforms: linux, windows
teaches: 20-10-what-time-it-is-at-boot
---

## Symptom

The robot stops by itself a minute after every boot, as the network comes up, or a watchdog never fires after the clock was put back.

## Cause

Setting the clock at 40 s made the wall clock jump 1794699960 s (56.9 years) in 0.2 s of real time. A timeout of 0.3 s in wall time expired and one in time since boot did not; with the clock put back an hour, wall time said fine and time since boot said expired.

## Fix

Measure every interval in time since boot.

```cpp
const bool expired = (now.mono - beat.mono) > timeout;
```
