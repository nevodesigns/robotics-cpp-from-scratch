id: E-SAFE-0013
title: A reading that was not evidence of anything
match: a reading that is not evidence is a blocked path, and a reading of nothing is not an empty one
match: the speed is limited by the range in the direction of travel, with the sign kept, and a command of zero stays zero
platforms: linux, windows
teaches: 19-05-a-speed-the-sensor-can-vouch-for
---

## Symptom

The robot drives into a pane of glass, a black wall, or something right in front of it, with a working sensor that returned zero, infinity or nothing.

## Cause

A reading of 0.0, 0.05 (blind zone), -1.0, NaN, an age that was not a number or 0.5 s old were all trusted as 0.0 m; a reading of infinity or 50 m was trusted as 1.5 m and no more. A reading of 2.0 m was 2.0 m.

## Fix

Say what a reading can vouch for, and treat the rest as blocked.

```cpp
if (std::isnan(r.range) || r.range < c.min_valid) return 0.0;
```
