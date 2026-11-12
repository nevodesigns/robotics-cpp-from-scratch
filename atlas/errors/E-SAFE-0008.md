id: E-SAFE-0008
title: A zero that was assumed to be a stop
match: zero is only safe where zero means stopped, so a channel whose neutral is not zero needs its own safe value
platforms: linux, windows
teaches: 19-03-what-the-output-does-on-its-own
---

## Symptom

On a dead controller the wheels do not stop, or a servo swings to one end, though every output was set to zero.

## Cause

An ESC channel whose neutral is a pulse of 1500 us was unsafe on the zero policy, because a pulse of 0 is no signal and the response to it belongs to the device. With its own safe value the pulse went to 1500 and the list was empty.

## Fix

Write the safe value of each channel down.

```cpp
channel.safe_value = 1500.0;
channel.on_failure = FailPolicy::safe_value;
```
