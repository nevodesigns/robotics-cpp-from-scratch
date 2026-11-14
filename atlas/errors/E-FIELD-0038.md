id: E-FIELD-0038
title: An inertia left at the default
match: the simulated robot spins up too fast or turns like a tanker
match: the link inertia is the default
match: the inertias are the ones of the shapes
platforms: linux, windows
teaches: 22-12-describing-a-robot-once
---

## Symptom

Turning in Gazebo is much quicker or slower than the same command in the built in model.

## Cause

The inertia was not given, or was for another shape. A box of 1 kg with sides 0.138, 0.178 and 0.172 m has an Ixx of 0.00511 kg m2; the links summed to 1.000 kg.

## Fix

Work the inertia out from the shape and the mass, and split the mass between the links so that it adds up.

```cpp
const Inertia bi = box_inertia(body_m, length, width, body_h);
```
