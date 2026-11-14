id: E-FIELD-0018
title: A GNSS jump believed
match: the position jumps by metres for one fix and the filter follows it
match: multipath makes a good fix jump
match: a GNSS receiver is a few metres off alone and a couple of centimetres when fixed, and sometimes jumps
platforms: linux, windows
teaches: 22-06-sensors-that-fail-like-the-field
---

## Symptom

The robot lurches towards a position a few metres away for one cycle, then back.

## Cause

About 1.03 percent of fixes jumped by metres even when the fix was fixed, which is good to 0.020 m.

## Fix

Gate each fix against the prediction before it is used, and treat an unlikely one as an outlier.

```cpp
if (innovation_distance > gate) return;  // reject this fix
```
