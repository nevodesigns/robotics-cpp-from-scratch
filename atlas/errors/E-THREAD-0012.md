id: E-THREAD-0012
title: A deadlock that never happened in testing
match: (never deadlocked in testing|a clean run proves nothing|passed every test and hung)
platforms: linux, windows
teaches: 07-09-two-locks-neither-moves
---

## Symptom

The code passed every test, and ran for a week on a bench. It hung on the first day in
the field. Rerun, it does not hang. The report is "it stopped once".

## Cause

Whether two threads deadlock depends on whether their timing lines up, and that depends on
everything else the machine is doing.

Measured: the same code, opposite lock order, two threads in a loop, 4000 passes, with
different amounts of unrelated work between passes, three runs each. Deadlocks seen:

| unrelated work between passes | run 1 | run 2 | run 3 |
|---|---|---|---|
| none | 129 | **3739** | 3387 |
| 10 us | 54 | 28 | 41 |
| 100 us | **0** | **0** | **0** |
| 1000 us | 0 | 3 | 16 |

The same bug showed thousands of times in one run and **none** at 100 us in all three
runs, then came back at a longer gap. A run that does not deadlock proves nothing, which
is what lesson 07-01 measured for a torn read. This one stops the robot.

## Fix

Do not wait for the hang. The **order** is the bug, and it is visible on one thread:
`A` then `B` here and `B` then `A` there, whether or not the two ever overlapped.

A level on every mutex, checked on every lock, reports the wrong order the first time it
is tried. Measured, two threads run one after the other so they could never have
overlapped: `drive` then `arm` was clean, `arm` then `drive` was reported as a violation,
with nothing deadlocked and no timing involved.

```cpp
rc::rt::LevelMutex drive(1);
rc::rt::LevelMutex arm(2);
```

This is what the Linux kernel's lock dependency checker does at a larger scale. It finds
the cycles that are made of the mutexes it knows about, and it finds them early.
