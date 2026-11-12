id: E-SAFE-0031
title: A self-test that stops at the first failure, or has no checks and passes
match: every check is run, a failure does not hide the next, and a throw is a failure
match: a test that checked nothing has not passed
platforms: linux, windows
teaches: 19-11-prove-it-still-works
---

## Symptom

The test reports one problem, it is fixed, and the next start up reports another, and so on for a week; or the test passes because its checks were never registered.

## Cause

Five checks were run in order: two passed and three failed (one returned false, one threw, one had no function) and all three names were reported. An empty list was not a pass and two passing checks were.

## Fix

Run every check, treat a throw and a missing check as failures, and fail an empty list.

```cpp
if (results.empty()) return false;
```
