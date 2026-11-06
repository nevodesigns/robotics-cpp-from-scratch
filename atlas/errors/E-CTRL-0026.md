id: E-CTRL-0026
title: One bandwidth chosen for noise and tracking at once
match: a push the model does not know about leaves a speed error of 2 d over w
match: more bandwidth tracks faster and passes more noise, and the two cross
platforms: linux, windows
teaches: 14-10-the-speed-you-cannot-measure
---

## Symptom

The estimate is quiet and late, or on time and noisy, and no value of the one setting gives both. Raising it fixes the lag and brings
the buzz back.

## Cause

Both gains come from one bandwidth w, and w sets the speed of the correction. A fast correction follows the truth closely, including
what the model got wrong, and also follows the noise. A slow one ignores both.

Measured, speed error rms in rad/s, noise only and tracking only:

| bandwidth | noise only | tracking only |
|---|---|---|
| 5 | 0.0002 | 2.5516 |
| 50 | 0.0053 | 0.5433 |
| 200 | 0.0490 | 0.1324 |
| 400 | 0.1675 | 0.0628 |

The noise rises with the bandwidth and the lag falls with it, and the two together were smallest at **200** of the values tried. A push the model does
not know about leaves a steady speed error of 2 d / w, measured 0.0395 rad/s against 0.0400 predicted for 1 rad/s^2 at 50.

## Fix

Choose w for the machine: no higher than the sensor's noise allows and no lower than the unmodelled accelerations need.

```cpp
const ObserverGains gains = observer_gains(bandwidth);   // 2 w and w squared
```

Measure both errors as the test does before settling on a number.
