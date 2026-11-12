id: E-FIELD-0046
title: A model setting taken from a data sheet and not from a run
match: the built in model responds faster than the robot in the other simulator
match: the time constant of the simulator is a round number nobody measured
platforms: linux, windows
teaches: 22-15-two-simulators-and-the-gap
---

## Symptom

The built in model's robot answers a command faster or slower than the one in Gazebo, by a fixed lag.

## Cause

The time constant was a round number, 0.05 s. Read from Gazebo's own step response it was 0.040 s.

## Fix

Read the parameter from a run of the thing you want to match, and set it.

```cpp
const double tau = response_time_constant(run_script(other, step, 0.01));
```
