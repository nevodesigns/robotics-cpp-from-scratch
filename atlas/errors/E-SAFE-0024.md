id: E-SAFE-0024
title: A thermal limit that is only a stop, and a reading that cannot be real
match: derating with the temperature holds a motor below its limit, and a hard stop at the limit makes it cycle
match: a reading that cannot be real is a stop, and it takes good readings to leave it
platforms: linux, windows
teaches: 19-08-the-battery-and-the-heat
---

## Symptom

A motor runs at its temperature limit all day, stopping and starting, or a sensor wire comes off and nothing notices.

## Cause

At full demand for an hour, derating peaked at 63.9 degrees with 0 stops and a mean power of 0.81; a hard stop peaked at 80.0 with 53 stops and a mean power of 0.85. A NaN, 200 degrees, -80 degrees and 2 V each stopped the monitor.

## Fix

Derate in a band below the limit, and treat an impossible reading as a stop.

```cpp
if (!plausible) { stopped_ = true; return out; }
```
