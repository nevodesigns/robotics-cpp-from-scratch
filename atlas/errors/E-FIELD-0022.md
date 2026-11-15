id: E-FIELD-0022
title: A writer that does not round trip
match: a scenario loaded and saved is not the same file
match: the second write differs from the first
match: writing a scenario and reading it back repeats it exactly
platforms: linux, windows
teaches: 22-07-a-field-scenario-file
---

## Symptom

Saving a scenario changes it, and a later run is not of the scenario that was loaded. Saving was added after loading worked.

## Cause

The writer dropped or reordered something. Text that has been parsed and written once must come back the same after a second parse and write.

A writer that drops a field, or prints a number with too few digits, makes a file that loads as a different scenario. The only check is to write, read and write again, and compare.

## Fix

Write in a fixed order with enough digits, and test the round trip.

```cpp
RC_CHECK(rc::field::write_scenario(rc::field::parse_scenario(once).scenario) == once);
```
