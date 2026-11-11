id: E-BUILD-0024
title: The LGPL modules of Qt linked statically
match: each way of getting it wrong is a finding of its own, with the module or the missing thing named
match: a program that links the usual modules dynamically and ships the licence text and an offer of source owes nothing more
platforms: linux, windows
teaches: 12-10-what-a-shipped-binary-owes
---

## Symptom

A program is shipped as one executable with Qt inside it, because that is easier to copy, and the person who has it cannot put another build of Qt in.

## Cause

The LGPL lets a program that is not free use Qt on the condition that the person who has the program can replace Qt. A static link needs the object files of the program for that. Measured: the same four modules linked dynamically had **0** findings and linked statically had **1**, `static-lgpl`.

## Fix

Link dynamically, and ship the Qt libraries beside the program:

```cmake
target_link_libraries(station PRIVATE Qt6::Core Qt6::Widgets)   # the shared libraries of Qt
```
