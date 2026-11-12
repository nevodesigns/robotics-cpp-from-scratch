id: E-SAFE-0023
title: A cut-off that fires on every acceleration, and again on every noisy sample
match: a stop on the voltage at the terminals stops the robot every time it draws current, and the corrected voltage does not
match: a resume point above the stop point turns a robot that chatters at the threshold into one that stops once
platforms: linux, windows
teaches: 19-08-the-battery-and-the-heat
---

## Symptom

The robot cuts out when it climbs or accelerates and comes back at once, or twitches for minutes near an empty battery.

## Cause

A 22.0 V battery at 15 A in pulses read 20.5 V at the terminals: a stop below 21 V changed state 60 times in a minute, the corrected voltage 0 times. At the stop voltage with noise 0.05 V, a resume equal to the stop gave 135 stops in 600 samples and a resume at 21.0 V gave 1.

## Fix

Add the sag back and resume above the stop.

```cpp
const double rest = volts + std::max(amps, 0.0) * cfg_.resistance;
```
