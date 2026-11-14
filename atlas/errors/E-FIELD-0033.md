id: E-FIELD-0033
title: A clamp that turns a NaN into the limit
match: a command that is not a number made the robot go at full speed
match: std::min with a NaN returns the limit
match: each broken drive is caught, by the promise it breaks
match: the joint buses: the real ones keep the promises and three broken ones are caught
platforms: linux, windows
teaches: 22-11-the-contract-test-for-an-interface
---

## Symptom

A sensor fault or a division by zero upstream produces a NaN command, and the robot sets off at its maximum speed.

## Cause

std::max(-limit, std::min(limit, x)) returns the limit when x is NaN, because NaN is not less than anything. The fake of 22-01 did this until the contract found it (2 promises broken by the plain clamp).

## Fix

Refuse a command that is not finite before clamping, and treat it as a stop.

```cpp
if (!std::isfinite(c.v) || !std::isfinite(c.w)) { stop(); return; }
```
