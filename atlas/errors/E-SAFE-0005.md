id: E-SAFE-0005
title: A robot that is allowed to move until something says otherwise
match: it starts stopped, and a loop nobody has reported on is an open loop
match: a reset needs a closed loop and a fresh heartbeat, and a refused reset changes nothing
match: an open loop stops at the next tick and outranks a link that was already lost
platforms: linux, windows
teaches: 19-02-the-stop-that-does-not-need-the-app
---

## Symptom

After a reboot, a reconnect or a reset of a wire, the robot moves by itself.

## Cause

The robot started stopped. With heartbeats and no word from the loop the reason was e-stop. After the link returned at 2.5 s the output was still 0. A reset was refused with an open loop and fresh heartbeat, with a closed loop and a stale one, and a refused reset changed nothing. A wire cut at 1.04 s gave 0 on the next call.

## Fix

Default to stopped, latch, and make the way out a call that can be refused.

```cpp
if (!line_closed_ || stale(now)) return false;
```
