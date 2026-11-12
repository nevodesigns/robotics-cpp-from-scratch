id: E-DEPLOY-0004
title: A toolchain file that was copied and edited by hand
match: a toolchain file is made from the triple, one setting to a line, and a missing sysroot or flags leave their lines out
match: a specification made from a valid triple, with a real absolute sysroot, has nothing wrong with it
platforms: linux, windows
teaches: 20-02-a-toolchain-file-that-cannot-lie
---

## Symptom

The cross build runs with no error and the binary is for the wrong processor, or the build finds a tool for the wrong machine.

## Cause

A toolchain file made from aarch64-linux-gnu with a sysroot and two flags was 13 lines, the same text every time; with no sysroot and no flags those lines were left out. A spec of a valid triple and an absolute sysroot had no problems.

## Fix

Generate the file from the triple and validate it.

```cpp
const auto spec = from_triple(triple, sysroot);
```
