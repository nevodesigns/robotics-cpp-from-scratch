id: E-CMAKE-0001
title: A change to the build had no effect
match: ninja: no work to do
platforms: linux, windows
teaches: 09-02-what-moc-generates
---

## Symptom

You changed CMakeLists.txt, or added a file, or added a macro that requires code
generation, and the build behaves exactly as it did before.

## Cause

The build system works from decisions taken at configure time: which files exist,
which need moc, which belong to which target. Editing a source file is picked up
automatically. Changing the shape of the build is not always.

## Fix

Reconfigure: cmake -S . -B build/default
When a build behaves impossibly, delete the build directory and configure again.
That takes seconds in this repository and eliminates stale state as a suspect.
