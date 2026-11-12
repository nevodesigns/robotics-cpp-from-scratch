id: E-FIELD-0050
title: A new board with no watchdog
match: the robot keeps driving after the program on the laptop crashed
match: the wheels do not stop when the commands stop
platforms: linux, windows
teaches: 22-16-a-bring-up-checklist-for-a-board
---

## Symptom

The robot drives on after the program that was commanding it has gone.

## Cause

The board holds its last command for ever unless its firmware drops it. The watchdog check gives one command and then silence, and found the board that had none.

## Fix

Turn on the watchdog in the firmware, and test it with the cable pulled.

```cpp
if (since_command_ > watchdog) wheels = 0;
```
