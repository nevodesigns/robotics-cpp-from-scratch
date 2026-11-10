id: E-BUILD-0018
title: A resolver that takes the newest and never backs up
match: when the newest version of something cannot be used, the resolver backs up to an older one
match: expected chosen_version
platforms: linux, windows
teaches: 04-13-two-things-need-different-versions
---

## Symptom

A project that could be built is declared impossible: "A 2.0 conflicts with B". Installing A 1.0 by hand makes it work. The resolver gave up at the first thing that did not fit.

## Cause

Taking the newest version of each package in turn is a guess, and a guess can be wrong. The newest A may need a version of C that B refuses. Reaching that point does not prove there is no answer, only that this choice is not one.

Measured: the newest A, 2.0.0, wanted C in `[2.0.0, 3.0.0)`, and B wanted C in `[1.0.0, 2.0.0)`. The resolver **backed up**: it gave up A 2.0.0 and chose A **1.0.0**, B 1.0.0 and C **1.7.0**, in 6 steps.

## Fix

Try the next candidate when a choice leads nowhere, and put back what the failed choice had asked for:

```cpp
wants = saved;          // forget this choice's requirements
chosen.erase(next);
```
