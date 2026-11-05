id: E-THREAD-0016
title: One task that threw, and a pool that slowly stopped working
match: (worker died|pool stopped making progress|one bad task|task threw and the worker)
platforms: linux, windows
teaches: 07-10-how-many-workers
---

## Symptom

A pool that worked starts finishing fewer tasks. Throughput falls over hours, with no error. Or the
whole process ends with `terminate called after throwing an instance of ...`, from a thread nobody
was watching.

## Cause

A task that throws, in a worker that does not catch it. Lesson 07-07 measured the outcome: an
exception that escapes a thread's function ends the process.

The quieter version is a worker that catches it at the top of its thread and exits. One bad task
retires one worker, and the pool is one smaller for ever. With four workers and a task that throws
once an hour, the pool is dead in a few hours and reports nothing until it is.

## Fix

Catch per task, at the one place tasks run, count it, and carry on:

```cpp
try { task(); } catch (...) { threw = true; }
...
if (threw) ++failed_;
++completed_;
--outstanding_;
```

Measured: 10 tasks on 2 workers, three of which threw, gave 10 run, **10 completed, 3 failed**, and
five further tasks submitted afterwards all ran. The workers were all still there.

`failed()` is not decoration. A pool that swallows exceptions without counting them hides bugs. A
task that throws once a day is information, and the count is where it goes. If a task's failure
matters to whoever submitted it, give the task a way to report: write the result into something the
caller holds, rather than relying on the pool to know what a failure means.
