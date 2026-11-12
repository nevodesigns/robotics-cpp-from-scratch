id: E-DEPLOY-0007
title: A mistake that only a cross build can show
match: a native build hides every one of these mistakes, because the target is the host
match: what runs where: the same architecture, a 32 bit x86 binary on a 64 bit x86 host, and nothing else
platforms: linux, windows
teaches: 20-03-run-here-ship-there
---

## Symptom

The project has built and passed its tests for a year, and the first cross build fails in four places at once.

## Cause

The graph with a generator and tests built for the target, a library built on the laptop and a host tool in the bundle had 0 problems with the host as the target. A 32 bit x86 binary runs on an x86_64 host, and aarch64, arm and mingw binaries do not.

## Fix

Check the graph for the two machines even when building natively.

```cpp
const auto issues = check(steps, edges, install, host, robot);
```
