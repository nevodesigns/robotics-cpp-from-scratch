id: E-SAFE-0004
title: A stop that waits for a message to notice there are none
match: when the heartbeat stops, the robot's own loop stops it one timeout later, with nobody sending a thing
match: a guard that only looks when a message arrives never stops when the messages stop
platforms: linux, windows
teaches: 19-02-the-stop-that-does-not-need-the-app
---

## Symptom

The application hangs or the network drops and the robot goes on driving at the last command, though there is a watchdog in the code.

## Cause

The watchdog checked the age of the last message only inside the handler for a message. When the messages stopped at 0.2 s its output at 5 s was 0.8. The stop path called by the robot's own loop every tick stopped at 1.3 s after a last heartbeat at 1.0 s with a timeout of 0.3 s, and gave 0 at 5 s.

## Fix

Evaluate the timeout in the loop that always runs, with the robot's clock.

```cpp
const double drive = stop.output(command, robot_clock());
```
