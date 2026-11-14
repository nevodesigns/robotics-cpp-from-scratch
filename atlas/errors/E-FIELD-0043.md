id: E-FIELD-0043
title: A comparison of runs of different lengths
match: the two simulators ended in different places because one ran for longer
match: the run is as long as the computer took
match: the program is written against Backend and is the same program for any of them
platforms: linux, windows
teaches: 22-14-the-same-program-another-simulator
---

## Symptom

The simulators disagree by a different amount each time the comparison is run.

## Cause

One of them ran in real time, so the length of the run depended on the machine. The Gazebo backend is paused and stepped, so a run is as long as it was asked to be.

## Fix

Advance simulated time explicitly in both and compare at equal times.

```cpp
request.set_multi_step(steps);  // then wait for the clock
```
