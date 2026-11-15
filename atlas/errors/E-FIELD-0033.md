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

A sensor fault or a division by zero upstream makes a NaN command, and the robot sets off at full speed. A division by zero or a sensor fault upstream.

## Cause

The clamp turned the NaN into the limit. std::max(-limit, std::min(limit, x)) returns the limit when x is NaN, because NaN is not less than anything. The fake drive of 22-01 did this until the contract found it: the plain clamp broke 2 promises.

Every comparison with NaN is false. min(limit, NaN) and max(-limit, NaN) return the limit or NaN depending on the order of the arguments, and in the common order the result is the limit: full speed.

## Fix

Refuse a command that is not finite before clamping, and treat it as a stop.

```cpp
if (!std::isfinite(c.v) || !std::isfinite(c.w)) { stop(); return; }
```
