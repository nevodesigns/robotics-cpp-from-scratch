id: E-TIME-0007
title: A task that adds a task while the scheduler is calling it
match: a task can schedule another, and it runs in the same run if it is due
match: expected times
platforms: linux, windows
teaches: 03-18-a-loop-you-can-run-in-a-millisecond
---

## Symptom

A scheduler works for a fixed set of tasks and crashes, or runs a task that is not there, once a task registers another while it runs. It is intermittent, because it depends on when the list happens to grow.

## Cause

Tasks are kept in a vector. A task that adds another can make the vector grow, which moves every element. If the scheduler is still calling a reference into it, the function object it is running has been moved from under it.

The scheduler copies the entry's period and task out before calling it. A task that schedules at +5 ms ran in the same run (times 10 ms and 15 ms, 2 events), and one at +500 ms waited for the next run and ran at 510 ms (3 events in all).

## Fix

Copy what is needed from the container before calling out, and never hold a reference into it across the call:

```cpp
const Nanoseconds period = entries_[index].period;
Task task = entries_[index].task;
task(when);
```
