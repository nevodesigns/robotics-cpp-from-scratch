id: E-SAFE-0030
title: A rule tested for the cases someone thought of
match: over every combination of ten conditions, a stop cause always stops, and adding a fault never makes the answer less strict
platforms: linux, windows
teaches: 19-10-one-place-that-says-stop
---

## Symptom

A new rule for one combination of conditions passes its test, and lets the robot go in a combination nobody wrote a test for.

## Cause

Over all 1024 combinations of ten conditions, 1008 were stopped, 15 degraded and 1 normal; 0 violations of the rule that a stop cause stops, and 0 of 10240 additions of a fault made the mode or the cap less strict.

## Fix

Test the property over every combination.

```cpp
for (int b = 0; b < (1 << 10); ++b) { /* add each condition and compare */ }
```
