id: E-FIELD-0044
title: A command that has not arrived when the world is stepped
match: the first second of a run in Gazebo does nothing
match: the twist is published and the world is stepped before the plugin has it
match: the built in backend starts at the scenario's start, and refuses a scenario with none
match: a straight drive goes where speed times time, less the lag, says, along the heading
match: a quarter turn in place turns a quarter of a turn, and the limits of the file hold a command back
match: the same program on Gazebo Sim: it starts, drives, turns and ends near where the built in model does
platforms: linux, windows
teaches: 22-14-the-same-program-another-simulator
---

## Symptom

The robot in Gazebo starts moving a moment later than the same program on the built in model.

## Cause

The command was a message on a topic and the world was stepped at once. The backend waits a moment after publishing.

## Fix

Let the message arrive before stepping, and check it by comparing the start of the trace on both backends.

```cpp
std::this_thread::sleep_for(std::chrono::milliseconds(50));
```
