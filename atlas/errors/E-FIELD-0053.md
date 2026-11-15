id: E-FIELD-0053
title: A report written by hand
match: the safety report says one thing and the table another
match: the document and the data disagree
match: the report is a table with a row for each hazard, and says how many problems are open
platforms: linux, windows
teaches: 22-17-a-safety-case-per-robot-type
---

## Symptom

The report that was reviewed is not the table that was tested. The report was edited after the tests ran.

## Cause

The report was typed by hand, so it can drift from the data.

A hand-written report is a second copy of the case. Each edit to one must be made to the other, and a copy that is not regenerated drifts.

## Fix

Generate the report from the case, and keep the case under test. A report of 13 lines was made from the data in one call, so it cannot disagree with it.

```cpp
write_file("safety-case.md", rc::field::to_markdown(c));
```
