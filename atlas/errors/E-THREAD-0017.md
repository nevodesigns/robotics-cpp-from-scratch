id: E-THREAD-0017
title: A task stranded between the check and the act
match: (stranded|check then act|time of check to time of use|TOCTOU)
platforms: linux, windows
teaches: 07-11-the-race-the-tests-passed
---

## Symptom

Something that was accepted is never done. A task submitted just as the system shut down is
counted as outstanding for ever, or a request that passed a check is acted on after the check
stopped being true.

## Cause

A check and the act it protects, with the lock released between them:

```cpp
{ lock; if (!accepting_) return false; ++counted_; }    // the check
race_point("submit.gap");                                // another thread can run shutdown here
{ lock; items_.push_back(id); }                          // the act
```

Both halves are individually locked and the whole is not atomic. If shutdown runs in the gap, it
processes the queue (empty), stops accepting, and returns. Then the push puts the task in a
queue nobody will read again.

This is the check-then-act race, and it is the most common concurrency bug there is. It looks
like careful code, because every access is locked.

Measured, shutdown injected at each point of one submit:

| point | buggy: stranded | fixed: stranded |
|---|---|---|
| before the call | 0 | 0 |
| **in the gap** | **1** | no gap |
| after the push | 0 | 0 |

Three places shutdown can run, two harmless and one not. Trying every point is a small
exhaustive search of the interleavings of two steps.

## Fix

Make the check and the act one step under one lock:

```cpp
{
  const std::lock_guard<std::mutex> held(mutex_);
  if (!accepting_) return false;
  ++counted_;
  items_.push_back(id);
}
```

Then shutdown comes before the lock, and the submit is refused, or after it, and the task is
already queued and gets drained. There is no gap to hook, which is the repair: the fixed row
never fires.

The rule: if a decision and the action it permits must agree, they hold the same lock for the
whole of both.
