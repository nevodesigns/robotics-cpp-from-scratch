id: E-FIELD-0020
title: A parser that stops at the first mistake
match: fixing one mistake in a file reveals the next, six times over
match: the parser reports every problem with its line number
match: every mistake in a bad file is reported once, with the line it is on
match: a file with no start and no duration says so, and a fault past the end is a problem
match: what is written back has every patch and every fault in it, not only the same text twice
platforms: linux, windows
teaches: 22-07-a-field-scenario-file
---

## Symptom

Each run of the tool reports one problem, and the file takes six runs to fix. The file was written by hand.

## Cause

The parser returned at the first bad line. A file with six mistakes gave 8 reports in one pass once they were collected (two whole-file rules failed as well).

A file can have several independent mistakes. A parser that stops at the first finds them one run at a time, and the person fixing the file pays a run for each.

## Fix

Collect the problems with their line numbers, check the whole-file rules at the end, and return all of them.

```cpp
problems.push_back(rc::field::ScenarioProblem{line, message});
```
