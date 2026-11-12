id: E-FIELD-0053
title: A report written by hand
match: the safety report says one thing and the table another
match: the document and the data disagree
platforms: linux, windows
teaches: 22-17-a-safety-case-per-robot-type
---

## Symptom

The report that was reviewed is not the table that was tested.

## Cause

The report was typed. A report of 13 lines was made from the data in one call, so it cannot disagree with it.

## Fix

Generate the report from the case, and keep the case under test.

```cpp
write_file("safety-case.md", to_markdown(c));
```
