id: E-SAFE-0021
title: Two sensors with the same weakness
match: two channels that are wrong in the same way agree, and the check does not see it
platforms: linux, windows
teaches: 19-07-two-sensors-that-must-agree
---

## Symptom

Both sensors say the path is clear, and agree, and it is not.

## Cause

With the truth at 3.0 m, two channels reading 1.5 m for 50 samples gave state ok and value 1.50 m: they share the same error, so their difference is zero.

## Fix

Make the second channel measure differently, and power and wire it separately.

```cpp
check.update(encoder_speed, visual_speed);
```
