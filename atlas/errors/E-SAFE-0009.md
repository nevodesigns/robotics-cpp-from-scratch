id: E-SAFE-0009
title: A brake that works only while everything around it works
match: a brake that is applied by a spring is engaged by a cut wire, a lost supply and a dead controller; one applied by power is engaged by none of them
match: the one case a spring does not cover is the output stage that holds, and a safe-value policy covers it
platforms: linux, windows
teaches: 19-03-what-the-output-does-on-its-own
---

## Symptom

The brake works in every test and not in the one failure it was fitted for: a cut wire, a blown fuse, a dead board.

## Cause

With the robot moving, a spring applied brake was engaged by a lost supply, a cut wire and a controller driving zero (3 of 4) and a power applied brake by none (0 of 4). The fourth case, an output stage holding 24 V, left the spring brake released until the coil channel was given a safe value of 0 V.

## Fix

Choose a brake that is applied by the loss of anything, and give its coil a safe value.

```cpp
coil.safe_value = 0.0;
coil.on_failure = FailPolicy::safe_value;
```
