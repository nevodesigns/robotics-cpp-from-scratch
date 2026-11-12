id: E-SAFE-0039
title: Redundancy that shares its cause
match: two channels are better than one, and the part of their failures that they share sets the limit
match: a hazard's likelihood after each fix, from the demands on a function and its chance of being dead, and a catastrophe stays at medium whatever
platforms: linux, windows
teaches: 19-13-how-likely-after-the-fix
---

## Symptom

The second channel is fitted and the risk assessment drops by a thousand, and in the field both channels fail together on one wet day.

## Cause

With a monthly test (3.64e-03) two channels were 275 times better at beta 0, 74 at 0.01, 19 at 0.05, 10 at 0.10 and 5 at 0.20. G2 at 20 demands a year went from likelihood 4 (unacceptable) to 3 (high) with a monthly test, to 2 (medium) with a hardware cut-off at beta 0.10, and with beta 0 gave 4 by the product and medium by the floor.

## Fix

Improve beta, and do not let the product take a catastrophe below medium.

```cpp
const double pfd = pfd_1oo2(single, beta);
```
