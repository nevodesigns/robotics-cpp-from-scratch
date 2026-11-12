id: E-DEPLOY-0032
title: Sums with no headroom, on a cool bench
match: six programs fit a four core board with headroom, and the numbers are what is left
match: in a hot enclosure the board runs at 60 percent, and the same robot no longer fits
platforms: linux, windows
teaches: 20-11-does-it-fit
---

## Symptom

The robot fits on the bench on a cool day and misses its deadlines, or has a program killed, on the warmest day of the year.

## Cause

Six programs used 2.60 of 2.80 CPU average, 4.90 of 5.00 peak, 2030 of 3277 MB and 8200 of 14746 MB of flash on a board of 4 cores. Throttled to 60 percent, 2.4 cores, the allowances became 1.68 and 3.00 and both were over.

## Fix

Check against allowances, with the board hot.

```cpp
check_fit(board, procs, release, 2, data, logs, Budget(), true);
```
