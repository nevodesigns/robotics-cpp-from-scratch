id: E-TEST-0032
title: A check loosened until it lets a fault through
match: a check that is too loose lets a fault through, and the table shows it
match: expected missed
platforms: linux, windows
teaches: 05-12-six-faults-and-a-matrix
---

## Symptom

A check that failed too often is loosened until it passes. It stops failing, and a fault that used to be caught is now invisible. Nobody notices, since nothing fails.

## Cause

A bound that is wide enough to be quiet is wide enough to miss things, and only the table of faults against checks shows which. Loosening the response check to "ends within 0.6 of the target" is a common way to stop it complaining.

Measured: with that loosened check, the weak actuator, which ends 0.2 from the target, was **missed by every check**, and **4** of the 6 faults were now seen by exactly one check where there were 3.

## Fix

Re-run the fault table after every change to a bound, and let a fault that is no longer caught fail the build:

```cpp
RC_CHECK(faults_missed(detection_matrix(loose, runs)).empty());
```
