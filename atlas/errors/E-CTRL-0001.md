id: E-CTRL-0001
title: Integral windup, the controller overshoots and takes seconds to recover
match: expected .*integral
platforms: linux, windows
teaches: 14-01-pid-from-scratch
---

## Symptom

After a period where the setpoint demanded more than the machine could deliver,
the controller keeps commanding full output long after the demand has dropped.
The system sails past its target.

## Cause

While the output is saturated the error never reaches zero, so the integral term
keeps accumulating. It can reach an enormous value, and it has to unwind before
the output comes back into range.

## Fix

Add anti windup: stop accumulating when the output is already saturated and the
error would push it further into saturation. Keep accumulating when the error
points back towards the usable range, so recovery stays quick.
