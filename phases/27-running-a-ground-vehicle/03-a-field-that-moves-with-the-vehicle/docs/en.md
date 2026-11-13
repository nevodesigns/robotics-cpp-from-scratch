# A Field That Moves With the Vehicle: Protective Fields Sized by Stopping Distance, Bent Round the Turn, and Switched in Sets

> The field is not a circle round the vehicle, it is the road the vehicle needs in order to stop.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 27-02, 19-04, 26-07

## The Problem

A person walks out in front of a vehicle. The vehicle's lidar sees them. What happens next is a question of **where**: if the person is close enough that the vehicle cannot stop before it reaches them, the vehicle has to stop now; if they are far enough, it can go on. A safety laser scanner on a driverless truck answers that with a **protective field**: a shape in front of the vehicle that, if anything is in it, stops the vehicle. The shape is not a circle round it. It is **the road the vehicle needs in order to stop**.

(Driverless industrial trucks are covered by a standard, ISO 3691-4, which asks for fields like these among much else. This lesson models the idea. It does not claim that code written from it would meet the standard, and a real safety function is a certified device, not a header.)

## The Concept

### As long as the stopping distance

The vehicle goes on at its speed while the scanner sees the thing and the brakes take hold (the **latency**), and then it brakes. The field is that distance plus a **margin**: `field_length = v latency + v squared / (2 a) + margin`. The same stopping distance as 19-04, with the speed the vehicle has **now**. A vehicle at 1.5 m/s that stopped when something entered its field stopped **0.20 m** short of it, which is the margin and no more.

Work the field out with the wrong latency and it is too short. With none, when the true latency was 0.3 s, the vehicle stopped **0.25 m past** where the thing was: it hit it, at a low speed, and it was the field that let it. See `E-VEH-0118`.

### Round the turn

The vehicle does not go straight in a turn. The road it needs is an arc, and a field in front of it, straight, misses the thing that is on the arc but to one side: a thing on the arc of a turn of radius 2 m, 40 degrees round, was not in the straight field and was in the swept one. The swept field is a ring of the width of the vehicle round the arc, as far along it as the length. The curvature comes from the steering (or, for a differential drive, the turn rate over the speed). See `E-VEH-0119`.

### A warning, and a stop

Twice the length is a **warning field**: something in it slows the vehicle to a **creep** (0.3 m/s here), because by the time it is in the stop field the vehicle should be going slowly enough that it hardly matters. Something in the **stop field** stops it.

### Field sets

A real device does not recompute the field every tick. It holds a few **sets**, each designed for a speed, and switches between them as the speed changes. The set must be designed for at least the speed the vehicle has: a vehicle at 1.1 m/s uses the set for 1.5, whose field is **1.46 times** as long as it needs, because that is the smallest set that is safe, and the vehicle stops for things further away than it needs to. A vehicle at 2.1 m/s on a device with sets up to 2.0 has no set that is big enough, and has to be **slowed** to a speed it has a set for. See `E-VEH-0120`.

## Build It

`in_swept_field` in `exercise/solution.hpp`.

```
rcpp verify 27-03
```

## Use It

`rc/drive/protective_field.hpp`.

```cpp
const rc::drive::FieldResult r = rc::drive::evaluate_field(scan_points, speed, curvature, field);
const double v = rc::drive::protective_speed(r, speed, field.creep);
```

**What this does not do.** It is a model, not a safety function: no redundancy, no check that the scanner works, no response time for the processing, no diagnostic coverage. The field is for one speed and one curvature at a time, and nothing here changes it as the vehicle accelerates or the curve tightens, so a real device needs a set for each combination, and the margin to cover the change in between. The width is fixed, so a vehicle with a load that hangs out is not covered. The deceleration is a number you give it, for the surface it is on (23-05, 19-04: on ice it is not the same). Nothing is said about what a detection of a person does as against one of a wall. The points are in the vehicle's frame, and where they came from (the scan, the ground removal of 24-02, the ageing of 24-07) is not the lesson.

## What Breaks First

- **A field that forgets the latency.** See `E-VEH-0118`.
- **A straight field on a turn.** See `E-VEH-0119`.
- **A speed with no field set.** See `E-VEH-0120`.

## Ship It

`rc/drive/protective_field.hpp` holds `FieldConfig`, `FieldPoint`, `FieldResult`, `field_length`, `in_swept_field`, `evaluate_field`, `protective_speed` and `select_field_set`.
