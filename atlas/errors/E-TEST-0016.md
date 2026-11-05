id: E-TEST-0016
title: A concurrency test that passed with the bug still in
match: (passed with the bug in|ran it a hundred times and it passed|the race the tests passed)
platforms: linux, windows
teaches: 07-11-the-race-the-tests-passed
---

## Symptom

A suite that exercises a threaded class, many times, with real threads, passes. Weeks
later the same class strands a task, loses an update, or hangs, in the field, once. Rerun
under the same test, it passes again.

## Cause

Running a race hopes the threads line up, and a race whose window is a few instructions
wide lines up rarely.

Measured, the worker pool's first draft reduced to its essential pattern, two real threads,
one submitting four tasks and one calling shutdown, 20000 trials at each of three delays,
three runs:

| | stranded a task in |
|---|---|
| buggy version, per setting and run | 11, 28, 22, 2, 8, 7, 9, 18, 14 of 20000 |
| fixed version | **0 of 180000** |

So running it a lot does find it: about **0.07 percent** of trials, and as few as 2 in 20000 in
a bad run. A suite that runs the race 100 times sees nothing about **93 percent** of the time
with the bug still in.

The count is not steady either. It changes with the delay, with the run and with whatever the
machine is doing, which is why a pass proves nothing and a failure cannot be reproduced.

## Fix

Stop hoping and make the threads line up. Name the places in the code where an interleaving
could matter, and let a test run an action at one of them:

```cpp
race_point("submit.gap");                                        // in the code under test
const RaceHook hook("submit.gap", [&] { intake.shutdown(); });   // in the test
```

Measured: with `shutdown()` run at the gap on one thread, the buggy version stranded a task
**300 times in 300**, where running it unaided did in about 0 of 300. No timing is involved, so
the test fails the same way every time and can be debugged.

Keep the stress run as well. It finds the interleavings you did not think to name. The two are
partners, and `E-THREAD-0011`'s level check is the third.
