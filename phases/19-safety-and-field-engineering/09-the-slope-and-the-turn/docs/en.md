# The Slope and the Turn: When a Vehicle Tips, the Speed a Curve Allows, and a Tilt Estimate That Is Not Fooled by a Turn

> A tilt sensor tells you the way the robot is leaning. It does not know why.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (an accelerometer and a gyro, simulated)
**Prerequisites:** 19-08, 19-04, 19-07, 18-07

## The Problem

G3 in lesson 19-01 was "rolls over on a slope", scored 9. A rollover is the quickest way a ground vehicle becomes a danger to the people around it, and the usual cause is not a slope the robot was ordered to climb. It is a **turn at a speed the robot's geometry does not allow**, or a payload that raised the centre of gravity, or a sideways slope met at an angle.

Two things are needed. A **limit** that prevents the turn that would tip the robot. And a **sensor-based monitor** that notices when the first has not been enough, which brings the problem of this lesson: the sensor for tilt is an accelerometer, and an accelerometer cannot tell gravity from the vehicle's own acceleration.

## The Concept

### Where a vehicle tips

A vehicle tips sideways about its outer wheels when the sideways pull on its centre of gravity, taken about them, outweighs its weight. That happens when the sideways acceleration exceeds `g * (track / 2) / height`, with `height` that of the centre of gravity. Standing still, it tips on a slope of `atan((track / 2) / height)`. For a track of 0.5 m:

| centre of gravity | tips on a slope of | tips at a sideways acceleration of |
|---|---|---|
| 0.3 m | 39.8 degrees | 8.18 m/s^2 |
| 0.4 m | 32.0 degrees | 6.13 m/s^2 |
| 0.5 m | 26.6 degrees | 4.91 m/s^2 |

A payload that lifts the centre of gravity from 0.3 m to 0.5 m takes **40 percent** off the sideways acceleration the vehicle can take, and 13 degrees off the slope. A track equal to twice the height tips at exactly 45 degrees. The geometry has to be measured with the real payload on: it is one of the things that changes between a test on the bench and a day in the field. See `E-SAFE-0025`.

### The speed a curve allows

On a curve of curvature `k` (1 over the radius) at speed `v` the sideways acceleration is `v^2 k`. So the fastest the vehicle may go, using a fraction of what would tip it, is `sqrt(factor * tip_accel / k)`. A turn of radius 1 m, using half: **2.02 m/s** with the centre of gravity at 0.3 m, and **1.57 m/s** (**77 percent**) with the payload that lifts it to 0.5 m. A radius of 4 m allows **4.04 m/s**, twice as fast for four times the radius: the speed goes as the square root of the radius. A straight line has no limit from this. This is what the planner of lesson 19-05 should be cut down by, as well as the sensor's range. See `E-SAFE-0025`.

### Tilt from an accelerometer

Standing still, an accelerometer feels only gravity, and the angle of the slope is `atan2(ay, az)` for roll and `atan2(-ax, sqrt(ay^2 + az^2))` for pitch. At 5, 20 and 35 degrees the formulas gave the angle exactly. But **an accelerometer feels acceleration too**, and a robot in a turn is accelerating sideways by design.

### Fooled by the turn

Level ground, a steady turn at 4 m/s^2 sideways for 10 s, the true roll zero, vibration of 0.3 m/s^2 on the accelerometer. The worst error in the second half:

| estimate | error |
|---|---|
| accelerometer alone | **27.3 degrees** |
| gyro and accelerometer, gate 15 percent of g | 22.1 degrees |
| gyro and accelerometer, gate 3 percent of g | 5.1 degrees |

The accelerometer alone reports a lean of 27 degrees that is not there, and a monitor with a stop at 25 degrees would stop the robot **in every turn**. The filter adds the gyro rate each step and only pulls towards the accelerometer when its magnitude is within a **gate** of g, which says it is probably feeling only gravity. But a turn at 4 m/s^2 leaves the magnitude only 8 percent over g, **inside** a gate of 15 percent, and the filter was fooled nearly as much. A gate of 3 percent ignored the turn, and the 5.1 degrees left are the vibration letting the odd sample through. See `E-SAFE-0026`.

### And a tight gate is not free

The gyro drifts: it has a bias, and integrated alone it goes wrong steadily. The accelerometer is what holds it. Standing still on level ground for 60 s, a gyro bias of 0.5 degrees per second and the same vibration:

| estimate | error after 60 s |
|---|---|
| gyro alone | 30.0 degrees |
| filter, gate 15 percent | 1.2 degrees |
| filter, gate 3 percent | 1.7 degrees |
| filter, gate 1 percent | 4.1 degrees |

A gate of 1 percent passes so few samples (the vibration moves the magnitude by about 3 percent) that the gyro's bias is corrected only now and then: 4.1 degrees. In the 4 m/s^2 turn the same gate gave an error of **0.9 degrees**. There is **no gate that is right for both**: a wide gate follows the accelerometer into every turn, and a narrow one trusts a drifting gyro. The choice is made from the robot's vibration and the longest turn it makes, and then checked on the robot. The better answer is not a better gate but a better sensor (a good gyro that drifts for minutes and not seconds) and a **limit on the turn** (the section above) that keeps the sideways acceleration low enough that the accelerometer is not so wrong. See `E-SAFE-0026`.

### The monitor

`TiltMonitor` has a warning level and a stop level. The warning has no debounce. The stop needs a **run** of samples at or past the stop level: with a warning at 15 degrees, a stop at 25 and a debounce of 3, one sample at 30 degrees was a warning, a 10 degree sample after it cleared the count, and three in a row, the last of them on the other side, were a stop. A NaN is a stop at once. See `E-SAFE-0027`.

## Build It

`static_stability_angle`, `tip_accel`, `max_turn_speed`, `accel_roll`, `accel_pitch`, `TiltEstimator::update` and `TiltMonitor::update` in `exercise/solution.hpp`.

```
rcpp verify 19-09
```

## Use It

`rc/safety/tilt.hpp` joins `rc::safety`.

```cpp
const double vturn = rc::safety::max_turn_speed(curvature, 0.5, cg_height_with_payload, 0.5);
rc::safety::TiltEstimator roll(2.0, 0.03);
const double angle = roll.update(gyro_x, ax, ay, az, dt);
if (monitor.update(angle) == rc::safety::TiltState::stop) trigger_stop();
```

**What this does not do.** The geometry is a rigid body tipping about a line, with no suspension, tyres that give or a load that shifts, all of which make a real vehicle tip **earlier**: the factor of 0.5 is there for that, and a number to be tested by tilting the real vehicle on a ramp with a person at the safety rope, not by trusting it. The estimator is one angle with a fixed gate, where the right tool is a full attitude filter (phase 16), and the monitor says stop and does not latch (that is for the supervisor). A robot that is **already tipping** cannot be saved by stopping, and the response to a tilt may be wrong: braking while tipping sideways is one thing and on a slope is another.

## What Breaks First

- **A payload that raised the centre of gravity, and nothing that knew.** See `E-SAFE-0025`.
- **A tilt alarm that fires in every turn.** See `E-SAFE-0026`.
- **A stop on a single bump, or a NaN let through.** See `E-SAFE-0027`.

## Ship It

`rc/safety/tilt.hpp` holds `kGravity`, `static_stability_angle`, `tip_accel`, `max_turn_speed`, `accel_roll`, `accel_pitch`, `TiltEstimator`, `TiltState` and `TiltMonitor`.
