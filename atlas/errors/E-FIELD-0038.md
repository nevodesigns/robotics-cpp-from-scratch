id: E-FIELD-0038
title: An inertia left at the default
match: the simulated robot spins up too fast or turns like a tanker
match: the link inertia is the default
match: the inertias are the ones of the shapes
platforms: linux, windows
teaches: 22-12-describing-a-robot-once
---

## Symptom

Turning in Gazebo is much quicker or slower than the same command in the built in model. The robot description used the simulator's default inertia.

## Cause

The inertia was left at the default, or was for another shape. A box of 1 kg with sides 0.138, 0.178 and 0.172 m has an Ixx of 0.00511 kg m2.

A body's resistance to turning depends on how its mass is spread, which is its inertia. A default of 1 kg m2 for a small robot is hundreds of times too large; a robot with that inertia turns like a ship.

## Fix

Work the inertia out from each link's shape and mass, and split the mass between the links so it adds up (here the links summed to 1.000 kg).

```cpp
const rc::field::Inertia bi = rc::field::box_inertia(body_m, length, width, body_h);
```
