id: E-CTRL-0028
title: A derivative filter chosen for quiet alone
match: the filter lags a moving joint, and the lag grows with the time constant
platforms: linux, windows
teaches: 14-11-the-noise-the-derivative-loves
---

## Symptom

The buzz is gone and the loop now rings or overshoots where it did not, because the derivative term arrives late.

## Cause

The filter delays everything it carries by about tf. Measured, rms error of the derivative of a noise free swing of half a radian at 1 Hz:

| tf (s) | rms (rad/s) |
|---|---|
| 0.000 | 0.0000 |
| 0.005 | 0.0697 |
| 0.020 | 0.2769 |
| 0.050 | 0.6652 |

A late damping term damps less, and a long enough delay turns it into a push the wrong way (lesson 14-07).

## Fix

Keep tf as short as the noise allows, a few times dt and well under the time the joint takes to move, and watch the lag as well as the chatter.

```cpp
const double tf = (kd / kp) / 5.0;   // Td / N with N of 5
```
