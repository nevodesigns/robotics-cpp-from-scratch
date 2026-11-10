id: E-BUILD-0017
title: Two requirements on one package that were not combined
match: what two ranges both allow is the larger low end up to the smaller high end
match: expected compare
platforms: linux, windows
teaches: 04-13-two-things-need-different-versions
---

## Symptom

Two libraries need a third. One wants it at least 1.0 and below 3.0, the other at least 1.5 and below 2.5. The first one installed gets its choice (2.9) and the second one then fails, though 2.4 would have suited both.

## Cause

Each requirement was treated on its own, one after the other. What the project needs is what all of them allow, which is the overlap: the larger of the low ends, up to the smaller of the high ends. A high end is excluded, so ranges that only touch overlap in nothing.

Measured: `[1.0, 3.0)` and `[1.5, 2.5)` gave `[1.5, 2.5)`. An open range and a closed one gave the closed one's high end. `[1.0, 2.0)` against `[2.0, 3.0)` was **empty**, and so was `[1.0, 1.5)` against `[2.0, 3.0)`.

## Fix

```cpp
r.low = compare(a.low, b.low) >= 0 ? a.low : b.low;
r.high = compare(a.high, b.high) <= 0 ? a.high : b.high;     // when both have one
```
