id: E-CTRL-0022
title: A speed limit that is checked after the controller has asked for more
match: the outer loop asks for a speed in proportion to the error, and never more than the limit
match: expected .*speed_command.* to be within
platforms: linux, windows
teaches: 14-09-two-loops-one-inside-the-other
---

## Symptom

A joint with a speed limit, because of a gearbox rating, a safety rule or a payload, exceeds it on large moves. The
limit is in the code: a clamp on the output, a saturation on the command. And the joint still reached 8 rad/s on a move
the limit says should top out at 3.

## Cause

In a single position loop the output is a torque. A speed limit has to be imposed on a different quantity from the one
the controller computes, so it is a clamp somewhere else, a rate limiter on the setpoint, or a check on the measurement
that cuts the torque. Each is a second mechanism fighting the first, and the controller has no knowledge of it.

Measured, a move of 2 radians: the position loop with P and D reached **8.04 rad/s** and the PID **8.83**, with the
motor saturated for most of the move. They go as fast as the torque allows.

## Fix

Make the speed the thing the outer loop outputs. The outer loop is a gain times the position error, clamped to the
limit, and that clamped value is the inner loop's setpoint. The limit is then not enforced afterwards: it holds
because the inner loop is never asked for more.

```cpp
double wanted = gains_.position_gain * (target - position);
if (wanted > gains_.speed_limit) wanted = gains_.speed_limit;
if (wanted < -gains_.speed_limit) wanted = -gains_.speed_limit;
```

The same move peaked at **3.04 rad/s** against a limit of 3: the inner loop overshoots its setpoint by a hundredth,
and that is the price of a tracking loop. A limit that must never be exceeded still needs a hard stop behind it, the
safety layer of lesson 14-02, and does not replace it.
