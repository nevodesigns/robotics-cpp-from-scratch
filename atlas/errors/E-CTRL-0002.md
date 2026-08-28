id: E-CTRL-0002
title: Derivative kick when the setpoint changes
match: expected .*after_setpoint_jump
platforms: linux, windows
teaches: 14-01-pid-from-scratch
---

## Symptom

The actuator jolts every time an operator changes the target, even though the
measurement has not moved.

## Cause

The derivative term is computed from the change in error. Changing the setpoint
changes the error instantly, and the derivative of an instant change is enormous
for one time step.

## Fix

Compute the derivative from the change in measurement instead, and negate it. The
measurement moves smoothly because it is physical, so no spike is possible. This
is called derivative on measurement and it is what production controllers do.
