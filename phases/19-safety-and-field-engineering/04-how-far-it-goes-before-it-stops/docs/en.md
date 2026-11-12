# How Far It Goes Before It Stops: Stopping Distance, the Speed a Sight Line Allows, and What Slope and Latency Do to Both

> The robot stops when the brake acts and not when you decide to stop it. Everything between the two is distance.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 19-03, 19-02, 19-01, 01-06

## The Problem

Lesson 19-02 stops the robot 0.3 s after the heartbeat is lost. Lesson 19-03 makes sure each output goes where it should when that happens. Neither says whether 0.3 s is **short enough**, and that is not a property of the code. It is a property of the robot's speed, and of what it is driving towards.

At 2 m/s, 0.3 s of waiting is 0.6 m of travel before the brakes have even been asked. This lesson turns every timeout in the stop path into a distance, and turns the distance the sensor can see into the speed the robot is allowed to go. G4 in lesson 19-01 was "cannot stop in the distance its sensor sees", scored 12.

## The Concept

### The distance, and a check against a robot that is actually stopped

During the latency the robot goes on at its speed. Then it brakes at a constant deceleration. So `stopping_distance = v t + v^2 / (2a)`. With a deceleration of 2 m/s^2 and a latency of 0.3 s, checked against a robot integrated step by step at 0.1 ms:

| speed | formula | simulated |
|---|---|---|
| 0.5 | 0.212 | 0.213 |
| 1.0 | 0.550 | 0.550 |
| 2.0 | 1.600 | 1.600 |
| 3.0 | 3.150 | 3.150 |
| 5.0 | 7.750 | 7.750 |

They agree to within 0.005 m, the step of the simulation. The sign of the speed does not matter, no braking at all gives infinity, and a negative latency is no latency. See `E-SAFE-0010`.

### The speed a sight line allows

Turn it round. If the sensor sees `d` metres clear, with a margin kept, the largest speed is the `v` for which the stopping distance is `d` minus the margin: `v = -a t + sqrt(a^2 t^2 + 2 a d)`. With a deceleration of 2, a latency of 0.4 s and a margin of 0.2 m:

| sight (m) | max speed (m/s) | stops in (m) |
|---|---|---|
| 0.2 | 0.000 | 0.000 |
| 0.5 | 0.556 | 0.300 |
| 1.0 | 1.160 | 0.800 |
| 2.0 | 2.000 | 1.800 |
| 5.0 | 3.654 | 4.800 |
| 10.0 | 5.512 | 9.800 |

Each one stops in exactly the sight line minus the margin. Ten metres does not allow twice the speed of five: the speed grows with the square root of the distance, which is why a robot that has "plenty of room" at a walking pace has none at a run. At a sight line no longer than the margin the speed is 0. See `E-SAFE-0010`.

### Latency is paid for in speed

The same 3 m sight line, a deceleration of 2 and a margin of 0.2, by latency:

| latency | max speed | share of the speed with none |
|---|---|---|
| 0.0 s | 3.347 | 100 percent |
| 0.1 s | 3.153 | 94 percent |
| 0.5 s | 2.493 | 74 percent |
| 1.0 s | 1.899 | 57 percent |

A second of latency costs 43 percent of the speed. Half a second, a number nobody notices in a user interface, costs a quarter. The cheapest way to make a robot safe at speed is often to shorten a timeout, not to fit a bigger brake.

### What the ground and the slope really give

A brake that can give 3 m/s^2 does not give it if the wheels slip. The ground can give at most `friction * g * cos(slope)`, the smaller of the two wins, and then gravity takes `g sin(slope)` away on a downhill slope. With brakes of 3 and a stop from 1.5 m/s at a latency of 0.3 s:

| ground | deceleration | stopping distance |
|---|---|---|
| dry, flat | 3.00 | 0.82 m |
| dry, 10 degrees downhill | 1.30 | 1.32 m |
| dry, 10 degrees uphill | 4.70 | 0.69 m |
| wet, flat | 2.94 | 0.83 m |
| wet, 10 degrees downhill | 1.19 | 1.39 m |
| wet, 17 degrees downhill | 0.00 | cannot stop |

A 10 degree downhill slope takes **57 percent** of the braking from the brakes on a dry floor, and 17 degrees of wet slope takes **all** of it: the robot cannot stop by braking, and the stopping distance is infinite. The deceleration is never below zero, and a robot that cannot stop must be handled by something other than the formula. The number to use in the speed limit is the **worst the machine will meet**, not the one the brake was tested at on a flat floor. See `E-SAFE-0011`.

### The latency is the whole chain

`total_latency` adds the sensor's delay, the stop path's timeout, a whole control tick (the event can happen just after one), and the brake's own delay. With a sensor of 0.1 s, a timeout of 0.3 s, a tick of 0.01 s and a brake of 0.15 s: **0.56 s**. At 2 m/s that is **1.12 m** before the brakes act, and 2.12 m in all. A 3 m sight line then allows **2.41 m/s**; cutting the timeout to 0.1 s allows **2.70 m/s**. The stop path's timeout from lesson 19-02 is not a number to choose for comfort: it is a share of the stopping distance, and it was set against one more thing than the link. See `E-SAFE-0012`.

## Build It

`stopping_distance`, `max_speed`, `effective_decel` and `total_latency` in `exercise/solution.hpp`.

```
rcpp verify 19-04
```

## Use It

`rc/safety/stopping.hpp` joins `rc::safety`.

```cpp
const double a = rc::safety::effective_decel(3.0, 0.3, slope_rad);       // the worst floor and slope it will meet
const double lat = rc::safety::total_latency(0.1, 0.3, 0.01, 0.15);
const double vmax = rc::safety::max_speed(sensor_range_m, a, lat, 0.2);
```

**What this does not do.** It assumes constant deceleration, a straight line, one friction number and a known latency; a real vehicle has a brake that takes time to build up, loads that shift when it brakes, a surface that changes, and a measured sight line that is shorter than the real one when the sensor sees a dark or glossy surface badly. **It gives a bound to be reduced by testing, not a number to be believed**: stop the real robot at the speed it gives, on the worst floor, and measure the distance. A person who stands in front of it is not a test. It also does not decide what the sight line is: the next lessons use it, from a sensor's range and from a map.

## What Breaks First

- **A timeout chosen without a speed.** See `E-SAFE-0010`.
- **A deceleration measured on a flat dry floor.** See `E-SAFE-0011`.
- **A latency that leaves out the stop path's own timeout, tick and brake.** See `E-SAFE-0012`.

## Ship It

`rc/safety/stopping.hpp` holds `stopping_distance`, `max_speed`, `effective_decel` and `total_latency`.
