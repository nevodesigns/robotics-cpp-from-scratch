id: E-DEPLOY-0011
title: A path in a bundle that leaves the directory
match: a path is safe to unpack only if it is relative, plain, and stays inside the directory
platforms: linux, windows
teaches: 20-04-what-is-in-the-box
---

## Symptom

An update overwrites a file outside the release directory, or the unpacking tool writes where it should not.

## Cause

Of 15 paths, 4 were accepted. Refused: ../etc/passwd, bin/../../x, /etc/passwd, bin//x, ./bin/x, bin/, an empty path, C:/x, one with a backslash, one with a space, and .., and a path of 256 characters.

## Fix

Refuse unsafe paths before any file is written.

```cpp
if (!safe_path(path)) return error;
```
