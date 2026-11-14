id: E-FIELD-0030
title: Collisions counted in samples
match: the collision count changes when the log rate changes
match: a stretch of samples inside an obstacle is one collision
match: a stretch of samples inside a post is one collision, and the clearance says how far inside
platforms: linux, windows
teaches: 22-10-scoring-a-run
---

## Symptom

The number of collisions doubles when the logging rate does, though the robot did the same thing.

## Cause

A path through two posts was 110 samples inside them. Counted as events it was 2 collisions.

## Fix

Count a collision when the clearance first goes below zero and not again until it has recovered.

```cpp
if (clear < 0.0 && !inside) ++s.collisions;
```
