id: E-FIELD-0022
title: A writer that does not round trip
match: a scenario loaded and saved is not the same file
match: the second write differs from the first
match: writing a scenario and reading it back repeats it exactly
platforms: linux, windows
teaches: 22-07-a-field-scenario-file
---

## Symptom

Saving a scenario changes it, and a later run is not of the scenario that was loaded.

## Cause

The writer dropped or reordered something. The text after a parse, a write, a parse and a write has to equal the text after one.

## Fix

Write in a fixed order, with enough digits, and test the round trip.

```cpp
RC_CHECK(write_scenario(parse_scenario(once).scenario) == once);
```
