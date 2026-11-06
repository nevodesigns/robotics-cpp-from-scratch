id: E-CTRL-0025
title: An observer that was not told what the motor was doing
match: told the acceleration it follows the motion, and without it the estimate lags
match: expected this to be true: without.observer
platforms: linux, windows
teaches: 14-10-the-speed-you-cannot-measure
---

## Symptom

The speed estimate is smooth and always late. It trails the true speed through every acceleration, and a loop that uses it
for damping reacts to what the joint did a moment ago.

## Cause

With no acceleration input, the prediction step assumes the speed stays as it is, so every change in speed has to arrive through
the correction, which is the position error, which only appears after the position has been wrong for a while. The estimate is a low pass
filter of the truth, with the lag that goes with one.

Measured on a noise free swing at a bandwidth of 50: told the acceleration, the speed error was **1.70e-03 rad/s**; told zero, **0.543**,
over 300 times worse, with no noise involved at all.

## Fix

Pass the acceleration that was commanded, converted from torque by the inertia the controller already knows:

```cpp
observer.update(measured_position, torque / inertia, dt);
```

The model does not need to be exact. A push it does not know about leaves a steady speed error of 2 d / w, which is small at a high enough bandwidth and
which the next entry explains.
