id: E-CTRL-0023
title: An integral that wound up while the motor could not respond
match: the torque is held to the limit, and the integral stops growing while it is
match: expected this to be true: guarded.integral
platforms: linux, windows
teaches: 14-09-two-loops-one-inside-the-other
---

## Symptom

A joint that is blocked, or told to move further than it can, and then released, keeps pushing the same way for a long
time before it responds to the new command. The longer it was blocked the longer it takes. A stuck joint that is then
told to go back can run the wrong way at full torque for seconds.

## Cause

The motor's torque is limited, so while it is saturated, a larger raw command changes nothing. The integral does not
know that. It keeps accumulating the error, as it is built to, and the accumulated value is a number that has to be
unwound by errors of the opposite sign before the output leaves the limit. This is windup, from lesson 14-01, and a
cascade has it in the inner loop as much as a single loop has it anywhere.

Measured, pushing against a joint that did not move, a speed error of 3 rad/s for five seconds with a motor that tops out
at 20 N m: an integral that always accumulates would reach 3 times 5, which is **15**. The guarded one read
**0.000**, because the motor was saturated from the first step.

## Fix

Conditional integration: do not integrate when the motor is saturated and the error is still pushing the same way.

```cpp
const bool saturated = torque != raw;
const bool pushing_further = saturated && ((raw > 0.0) == (speed_error > 0.0));
if (!pushing_further) integral_ += speed_error * dt;
```

When the error pulls back, the integral is allowed to move, so it still unwinds, and when the target reverses the loop
answers at once: the test reverses the target after those five seconds and the torque goes straight to the opposite
limit. The simplest alternative, clamping the integral to a fixed range, bounds the windup and does not remove it.
