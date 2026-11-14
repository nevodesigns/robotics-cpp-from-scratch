id: E-FIELD-0042
title: A program tested on one simulator
match: the program works in the simulator and not in the other one
match: the behaviour depends on a habit of the simulator
match: the program is written against Backend and is the same program for any of them
platforms: linux, windows
teaches: 22-14-the-same-program-another-simulator
---

## Symptom

A controller that is perfect in the simulator it was written in is wrong in the next.

## Cause

It had only been run against one. The same script now runs against any Backend, and the built in model ended 5 s at 0.2 m/s at y = 2.990.

## Fix

Write the program against an interface and run it on two backends.

```cpp
Backend& backend = ...;  // a program takes this, never a concrete simulator
```
