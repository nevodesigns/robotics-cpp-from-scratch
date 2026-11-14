id: E-DEPLOY-0008
title: Something the build runs, built for the robot
match: the same graph built for the robot has four, each named
platforms: linux, windows
teaches: 20-03-run-here-ship-there
---

## Symptom

The build stops with an exec format error, or a cannot execute binary file, in the middle of generating code.

## Cause

Built on x86_64 for aarch64 the graph had 4 problems: msg_gen and unit_tests built for the target but run during the build, robot_node linking libmsgs built for the other machine, and calibrate, a host binary, in the bundle.

## Fix

Build what the build runs for the host.

```text
Steps built for the host: msg_gen
```
