id: E-BUILD-0006
title: A release candidate treated as newer than its release
match: a release candidate comes before its release, and the pre-releases sort in the order the spec gives
match: expected compare
platforms: linux, windows
teaches: 04-09-which-version-is-newer
---

## Symptom

A deployment picks 1.0.0-rc.1 over 1.0.0 because it sorts later as text, or picks beta.2 over beta.11. A tool that is supposed to give a stable release hands out a candidate.

## Cause

The part after the dash is a pre-release label, and the rules for ordering it are not alphabetical. A version with a label is older than the same numbers without one. Between two labels the dot-separated identifiers are compared one at a time, numbers as numbers, a number lower than a word.

Measured: `"1.0.0-beta.11" < "1.0.0-beta.2"` as text is **true**, which is wrong, and `compare` gave **1**. Eight versions from the specification's own example, shuffled and sorted with `compare`, came out in the published order.

## Fix

A release outranks a pre-release of the same numbers, and a pre-release is compared identifier by identifier:

```cpp
if (a.pre.empty()) return 1;
if (b.pre.empty()) return -1;
```
