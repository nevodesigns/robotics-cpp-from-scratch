id: E-CTRL-0035
title: A gain table with too few knots
match: a schedule made at a few poses does the same between them, and more knots do better
platforms: linux, windows
teaches: 14-13-gains-that-follow-the-pose
---

## Symptom

The arm behaves at the poses the table was made for and speeds up or slows down between them.

## Cause

The inertia is curved and the table joins its knots with straight lines. Settling time over 21 poses, fastest to slowest:

| knots | fastest to slowest (s) | spread (s) |
|---|---|---|
| 2 | 0.35 to 0.74 | 0.394 |
| 3 | 0.67 to 0.74 | 0.075 |
| 5 | 0.72 to 0.74 | 0.022 |

## Fix

Add knots where the table is most curved, and test at poses that are not knots.

```cpp
const GainSchedule schedule({0.0, 0.25, 0.5, 0.75, 1.0}, 8.0, 0.6);
```
