id: E-SAFE-0007
title: An output that keeps driving after the controller has died
match: an output stage left on its default keeps driving every channel that was moving when the controller died
match: hold keeps the last command, zero gives zero, and a safe value gives the channel's own
platforms: linux, windows
teaches: 19-03-what-the-output-does-on-its-own
---

## Symptom

The control program crashes or the board resets, and the wheels go on turning at the last speed until the battery dies or something is hit.

## Cause

With the five channels on the default (hold) and the controller dead, drive_left, drive_right, steering and brake_coil were not at their safe value; the horn, at 0, was. Set to zero, the list was empty.

## Fix

Choose a policy per channel and test the list.

```cpp
RC_CHECK(unsafe_on_failure(channels, last).empty());
```
