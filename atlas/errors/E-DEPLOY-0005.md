id: E-DEPLOY-0005
title: -march=native in a cross build
match: the settings that build for the wrong machine without an error are each named
platforms: linux, windows
teaches: 20-02-a-toolchain-file-that-cannot-lie
---

## Symptom

The robot dies with an illegal instruction, on code that ran on the laptop and compiled without a warning.

## Cause

A copied spec gave 6 problems: an empty compiler, a system name and a processor the triple does not imply, a relative sysroot, and -march=native and -mtune=native, each as the machine doing the build. A Windows sysroot with a backslash and a sysroot that does not exist gave one problem each.

## Fix

Name the machine in the flags.

```cpp
spec.cxx_flags = {mcpu_flag};
```
