id: E-CTRL-0032
title: A loop rate chosen at the edge of what works
match: a period chosen as a share of the limit costs more overshoot the larger the share
platforms: linux, windows
teaches: 14-12-the-rate-the-loop-runs-at
---

## Symptom

The loop works on the bench and fails in the field, or fails once something else makes a tick late.

## Cause

A loop with no margin has none to spare for a late tick or a sensor lag. Overshoot against a 1 ms loop, at kp 100 and kd 10:

| share of the limit | overshoot | against 1 ms |
|---|---|---|
| 0.05 | 0.164 | 1.05 |
| 0.10 | 0.175 | 1.12 |
| 0.25 | 0.219 | 1.40 |
| 0.50 | 0.353 | 2.26 |

## Fix

Run at a tenth to a twentieth of the limit, and check the worst late tick against the limit as well.

```cpp
const double period = period_for(kp, kd, 0.05);
```
