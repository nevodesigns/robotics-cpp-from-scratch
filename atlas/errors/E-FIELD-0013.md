id: E-FIELD-0013
title: A speed limit chosen for one surface
match: the vehicle overshoots its stop on a slippery surface that the limits were not set for
match: the stopping distance on ice is many times that on tarmac
match: the stopping distance on flat ground is v squared over twice g times \(mu \+ crr\), and the surface changes it several times over
match: patches override the base in the order they were added, and an edge belongs to one patch only
platforms: linux, windows
teaches: 22-05-terrain-and-friction
---

## Symptom

Stops that are exact on the test floor are metres long in the field. The limit was tuned on a dry floor indoors.

## Cause

The speed limit was chosen for one surface. Stopping distance goes as one over the friction: from 2 m/s it was 0.250 m on tarmac and 1.853 m on ice.

Braking is limited by the tyres' grip, which is the friction times the weight. Stopping distance is speed squared over twice the braking, so a surface with an eighth of the friction gives roughly eight times the distance.

## Fix

Take the braking limit from the surface under the vehicle, and plan the stop with it.

```cpp
const double stop = rc::field::stopping_distance(speed, rc::field::traction(world.at(x, y), heading).brake);
```
