id: E-FIELD-0042
title: A program tested on one simulator
match: the program works in the simulator and not in the other one
match: the behaviour depends on a habit of the simulator
match: the program is written against Backend and is the same program for any of them
platforms: linux, windows
teaches: 22-14-the-same-program-another-simulator
---

## Symptom

A controller that is perfect in the simulator it was written in is wrong in the next. It was developed in one simulator and moved to another.

## Cause

It had only been run against one simulator, so the simulator's quirks were part of what it was tuned to.

Each simulator has its own contact model, friction and timing. A controller developed in one is tuned to its quirks, and the second shows which of its behaviour was the controller's and which the simulator's.

## Fix

Write the program against an interface and run it on two backends. The same script runs against any Backend; on the built in model, 5 s at 0.2 m/s ended at y = 2.990.

```cpp
std::vector<rc::field::BackendPose> trace = rc::field::run_script(backend, script, 0.01);   // backend is any Backend&
```
