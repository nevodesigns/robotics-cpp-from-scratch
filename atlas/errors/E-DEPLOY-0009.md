id: E-DEPLOY-0009
title: A host binary in the bundle, or a host library in the robot
match: with the generator built for the host, the library built for the target, the tests left to run on the robot or an emulator, and only target binaries in the bundle, there is nothing left
match: a name that is not a step, and a triple that cannot be read, are problems and not crashes
platforms: linux, windows
teaches: 20-03-run-here-ship-there
---

## Symptom

The bundle contains a binary the robot cannot run, and nobody knows until someone tries.

## Cause

With the generator built for the host, the library for the target, the tests not run by the build and only robot_node installed, the graph had 0 problems. A link or an install naming something that is not a step gave an issue on that name, and an unreadable triple gave one issue.

## Fix

Install only target binaries.

```cpp
install = {robot_node};
```
