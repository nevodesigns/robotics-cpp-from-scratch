id: E-TEST-0026
title: A failing test that says only that the calls were different
match: both kinds of test catch the bugs, but only the state test says what the bug did
match: expected forgot.violations
platforms: linux, windows
teaches: 05-10-checking-how-not-what
---

## Symptom

A test fails and the message is "expected: enable, set 25, set 50, set 75, set 100; got: set 25, set 50, set 75, set 100". It takes a long look to see that the program never enabled the motor, and the message does not say what that did.

## Cause

A list of calls can only say that two lists differ. The state says what happened: how many speeds the driver ignored and where the motor ended up. A failure that names the consequence is one a person can act on.

Measured: all 3 bugs were caught by both kinds of test. Never enabling the driver left **4** speeds ignored and a final speed of 0. A last step past the target and the limit left **1** speed refused and a final speed of **0.75**.

## Fix

Check the invariants that matter, and let the fake's counters carry the explanation:

```cpp
RC_CHECK_EQ(port.violations(), 0);
RC_CHECK_NEAR(port.speed(), 1.0, 1e-12);
```
