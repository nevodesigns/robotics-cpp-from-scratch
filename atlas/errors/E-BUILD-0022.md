id: E-BUILD-0022
title: Two filters that share one state
match: a copy has its own state, assignment copies it, and a move hands it on
match: expected b.update
platforms: linux, windows
teaches: 04-14-the-header-that-changed-and-took-everything-with-it
---

## Symptom

A filter is copied for a second sensor, and each sensor's smoothed value is pulled by the other's readings. Or the program does not compile at all, because a class that owns its inside through a `unique_ptr` cannot be copied.

## Cause

A `unique_ptr` is one owner and has no copy. A class that holds one has no copy either, until it is given one. The easy fixes are wrong: sharing the pointer makes two objects change one state, and deleting the copy makes the class unusable in a container.

The right copy makes a new inside from the old one. Measured: a filter at 15 after two readings was copied, and a reading of 20 gave the copy 17.5, where the original, given 0, gave **7.5**: it had not moved. Assigning one to another took the first's alpha and value, assigning a filter to itself changed nothing, and a move handed the state on.

## Fix

```cpp
inline Smoother::Smoother(const Smoother& other) : impl_(new Impl(*other.impl_)) {}
```
