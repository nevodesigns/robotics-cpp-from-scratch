id: E-BUILD-0005
title: Versions compared as text
match: 1.9.0 is older than 1.10.0, though as text it sorts after
match: expected compare
platforms: linux, windows
teaches: 04-09-which-version-is-newer
---

## Symptom

A script that picks the newest installed version picks 1.9 over 1.10. Everything works until the minor number reaches two digits, and then an old library is used and nobody can see why.

## Cause

As text, `"1.10.0"` comes before `"1.9.0"` because `'1'` is less than `'9'`. Digits compare one character at a time and the number ten starts with a one.

Measured: `"1.9.0" < "1.10.0"` as text is **false**, and `compare` of the two versions gives **-1**, meaning the first is older.

## Fix

Split the version into numbers and compare those, major first, then minor, then patch:

```cpp
if (a.minor != b.minor) return a.minor < b.minor ? -1 : 1;
```
