id: E-CTRL-0034
title: A gain table that extrapolates past its ends
match: outside the table the schedule holds its end gains instead of extrapolating
match: a table lookup joins its points with lines and holds the ends beyond them
platforms: linux, windows
teaches: 14-13-gains-that-follow-the-pose
---

## Symptom

At a pose just past the last knot the gains are huge, or negative, and the joint jumps.

## Cause

A line carried on beyond the last two knots keeps rising or falling. A pose outside the table was never designed for, and the gains found for it are an accident. The gains at the ends of the lesson's table were kp 32.0 at the folded end and 192.0 at the extended one, and a straight extension from three poses back would go far past both.

## Fix

Clamp: hold the end values beyond the first and last knots.

```cpp
if (x <= xs[0]) return ys[0];
if (x >= xs[n - 1]) return ys[n - 1];
```
