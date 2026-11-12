id: E-DEPLOY-0033
title: A release that was never checked against the flash it has to share
match: adding a map server of 1500 MB pushes memory over, and each over-budget resource is named with what was used and what was allowed
match: two slots halve the release that fits, and a release that is too big for them is named before it is built
platforms: linux, windows
teaches: 20-11-does-it-fit
---

## Symptom

The update that worked for a year stops fitting, or one new program takes memory past the point where the system starts killing things.

## Cause

A map server of 1500 MB took memory to 3530 MB against 3277 allowed, with everything else still fitting. With 16 GB of flash, 2 GB of data and 200 MB of logs, a release of up to 6273 MB fits with two slots and 12546 MB with one; 100 MB past that gave a flash finding.

## Fix

Fail the build when the release is over the number.

```cpp
const double limit = max_release_mb(board, 2, data, logs, Budget());
```
