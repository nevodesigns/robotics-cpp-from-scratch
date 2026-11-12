id: E-SAFE-0037
title: An unknown probability that was counted as small
match: and multiplies, or is one minus none, nothing known is certain, and the empty cases are the safe ones
platforms: linux, windows
teaches: 19-13-how-likely-after-the-fix
---

## Symptom

The assessment says the stop fails once in a million demands, and the number for one of the protections was never filled in.

## Cause

A probability that was not a number was taken as 1: p_and with 0.001 and a NaN gave 0.001, and p_or gave 1. An and of nothing was 1 and an or of nothing 0; 1.7 and -0.3 were brought to 1 and 0.

## Fix

Take what is not known as certain.

```cpp
if (std::isnan(p)) return 1.0;
```
