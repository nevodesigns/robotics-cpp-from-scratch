id: E-MATH-0020
title: A measurement accepted or rejected by its distance in metres
match: a point can be close in metres and far in spreads, and the other way round
match: expected this to be true: d_across
platforms: linux, windows
teaches: 06-11-the-shape-of-the-doubt
---

## Symptom

A filter rejects any measurement more than half a metre from its prediction. In one direction it is too strict, since the prediction is uncertain by two metres along the corridor, and in the other it is too loose, since the uncertainty across it is a few centimetres.

## Cause

Distance in metres treats every direction alike. An uncertainty is not the same in every direction, and a point's distance from the centre only means something measured in the cloud's own spreads.

Measured, for a cloud with spreads 2 and 1 and a correlation of 0.8: a point **2.2** metres out along the cloud had a squared distance of **1.00**, well inside; a point **2.00** metres out across it had **12.38**, far outside the limit of 5.991. A point one spread out along an axis of an uncorrelated cloud has a squared distance of exactly 1.

## Fix

Gate on the squared Mahalanobis distance:

```cpp
(c.yy * dx * dx - 2.0 * c.xy * dx * dy + c.xx * dy * dy) / det
```
