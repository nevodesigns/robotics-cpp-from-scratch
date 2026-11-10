id: E-TEST-0024
title: A test that compares the calls made with a list
match: a test of the calls breaks on harmless changes, and a test of the state does not
match: expected interaction_broken
platforms: linux, windows
teaches: 05-10-checking-how-not-what
---

## Symptom

A refactor that changes nothing the motor does turns a dozen tests red. Each failure is a list of calls that differs from the expected list by one entry, and the fix, every time, is to edit the list.

## Cause

A test that records the calls and compares them with a list is written against one implementation. Any change to the order, the number or the repetition of calls fails it, whether or not the motor ends up in a different place. The test asserts how, and the requirement was what.

Measured: five changes that do not matter, a ramp in 8 steps or in 2, a second `enable`, the final speed set again, and a `disable` first. The test of the calls failed **5 of 5**. The test of the motor's state failed **0**.

## Fix

Put the expectation on the state the call sequence leads to:

```cpp
SimPort port(1.0);
bring_up(port, 1.0, style);
RC_CHECK(port.enabled() && port.speed() == 1.0 && port.violations() == 0);
```
