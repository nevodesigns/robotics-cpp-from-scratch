# Units and Limits That Cannot Be Mixed Up: Speed, Rate, Acceleration and Lateral Acceleration, Shaped Before They Reach a Motor

> A limit written in the wrong unit is not a small error, it is a robot that is thirteen times too brave.

**Type:** Build
**Time:** about 70 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-01, 21-10

## The Problem

A vehicle has limits, and the numbers for them arrive from places that do not agree: a data sheet in km/h, a planner in m/s, a joystick in percent, a gyro in degrees per second. A limit that is read in the wrong unit does not give a slightly wrong robot. It gives a stopping distance that is **8.331 m instead of 0.642 m**, which is the same code with one number from the wrong column. And a limit that is only a cap on speed is not enough: a vehicle that may do 1 m/s in a straight line may not do it round a tight turn.

## The Concept

### The unit in the type

`Speed` and `Rate` hold one number and **no way to make one from a bare double**: `Speed::from_kmh(5.0)` and `Speed::from_mps(5.0)` are different words, and 5 km/h is **1.389 m/s**. The constructor is where the unit is said, once, and every use after that is in one unit. It costs nothing at run time. It is the idea of strong types, taken into the vehicle's own vocabulary: the compiler cannot know that 5 is km/h, but it can make you say so. See `E-FIELD-0007`.

### Five limits and what they do

`VehicleLimits` holds speed, turn rate, acceleration, **braking** (allowed to be stronger, and nearly always is), turn acceleration and **lateral acceleration**. `shape()` applies them in an order that matters:

1. The wanted speed and turn rate are clamped to their maxima.
2. The change from what the vehicle did last step is limited: slowly for speeding up, harder for braking.
3. The speed is cut, if need be, so that speed times turn rate stays within the lateral limit. That is what stops a vehicle tipping or sliding in a turn (the same idea as lesson 19-09, here as a shaper in front of any controller).

With 0.5 m/s2 of acceleration a ramp to 1 m/s took **2.000 s**; a stop from 1 m/s with 1.5 m/s2 of braking took **0.667 s and 0.333 m**, a third of the time and a third of the distance of the ramp. 1 m/s asked for with a turn rate of 2 rad/s was cut to **0.750 m/s**, which is a radius of 0.375 m, and a straight line was left alone. See `E-FIELD-0008`.

### The unit mistake, measured

A speed limit of 5 meant as km/h and read as m/s. The vehicle stopped from 1.39 m/s in **0.642 m** and from 5 m/s in **8.331 m**, **13.0 times** as far. Everything downstream was right: the planner, the stopping margin, the tests, all written for the speed that was believed. See `E-FIELD-0007`.

## Build It

`shape` in `exercise/solution.hpp`.

```
rcpp verify 22-03
```

## Use It

`rc/field/limits.hpp` opens the module `rc::field`.

```cpp
rc::field::VehicleLimits lim;
lim.max_v = rc::field::Speed::from_kmh(5.0);
const rc::field::Shaped s = rc::field::shape(want_v, want_w, prev_v, prev_w, 0.02, lim);
drive.command({s.v, s.w});
```

**What this does not do.** The lateral cut is instant, so it can brake harder than the braking limit says; a real shaper would plan the slowdown before the bend. It limits the command, not the vehicle: a vehicle on loose ground may not follow it (the next lessons), and nothing here knows about a slope. And it gives each limit one number, where a real vehicle's depend on load and battery.

## What Breaks First

- **A limit read in the wrong unit.** See `E-FIELD-0007`.
- **A cap on speed that ignores the turn.** See `E-FIELD-0008`.
- **A braking limit assumed equal to the acceleration limit.** See `E-FIELD-0009`.

## Ship It

`rc/field/limits.hpp` holds `Speed`, `Rate`, `VehicleLimits`, `Shaped`, `clamp_to` and `shape`.
