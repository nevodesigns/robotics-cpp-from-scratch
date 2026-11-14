# Staying Far Enough From a Person: The Protective Separation Distance, the Speed It Allows, and What Each Term Is For

> The distance at which an arm must stop is the distance a person covers while it is reacting and braking, plus everything it cannot know.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 31-05, 30-08, 29-08, 19-01

## The Problem

31-04 felt contact after it happened. A person working next to an arm should never be touched at all, or touched only gently, which means the arm has to stop before it reaches them, and the arm has to know how soon that is. The idea of speed and separation monitoring (in ISO/TS 15066 and ISO 13855) is to keep the separation between the person and the robot at least as large as the distance in which the robot can stop, counting that the person is moving too.

This lesson is a model of the idea, to see what each term does. It is not a safety function: a real one is designed, measured and validated against the current standards (19-01 and the lessons after it say what that takes).

## The Concept

### The protective separation distance

When the person enters the zone and the system senses it, three things happen before the robot is at rest. It takes a reaction time t_r (sensing, processing, the brakes starting); it then brakes for t_s = v_r / decel; and the person walks on all that time. So the separation needed is

v_h (t_r + t_s) (the person's travel) + v_r t_r (the robot's travel before braking) + v_r squared over twice decel (the robot's stopping distance) + c + z_d + z_r (the intrusion distance, and the uncertainty in where the person and the robot are).

With a person at 1.6 m/s, 100 ms of reaction, 5 m/s2 of braking and 17 cm of intrusion and uncertainty (all typical), a robot at 1.0 m/s needed 0.85 m and a robot at rest 0.33 m: even a robot that is not moving needs room, because the person is. See `E-ARM-0091`.

### The speed a distance allows

Turn it round: at a given separation the robot may go at most as fast as makes its protective distance equal that separation. At 1.0 m it could go at 1.23 m/s and at 2.0 m at 2.49 m/s; closer than the distance for standing still it must stop. This is a speed limit that depends on the person's distance, and it is what a cell actually does: the arm slows as a person comes near. See `E-ARM-0092`.

### Each term earns its place

In a simulation of a person walking at the robot, a robot that braked at the full protective distance came to rest 0.17 m from the person (at least the intrusion and uncertainty it had kept). One that left out the person's motion (it was designed as if people stood still) came to rest at -0.31 m, which is negative: they met. One that left out the reaction time came to rest -0.09 m from the person, which is also negative: they met, only a little later than before. The terms are not decoration. See `E-ARM-0093`.

## Build It

`max_speed` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- Protective_distance is a quadratic in v_r (a = 1/(2 decel), b = v_h/decel + t_r, c = everything else less the separation); take the positive root, 0 if the constant term is not negative.

As shipped, 1 of the 3 tests fail, and these are the ones your code has to turn green:

- The greatest speed a separation allows is the inverse, and is zero when standing still does not fit.

The other 2 pass already, because they test the parts that are given.

```
rcpp verify 31-06
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/arm/ssm.hpp`.

```cpp
rc::arm::SsmParams p;
const double v_limit = rc::arm::max_speed(p, separation_to_nearest_person);
```

**What this does not do.** It is one dimensional: the person and the robot approach along a line, and a real cell takes the direction (only the part of the robot's speed toward the person counts) and every point of the robot's body. The person walks at a constant speed and the robot brakes at a constant deceleration from the moment its brakes start. The sensors are perfect up to the uncertainties given, and their latency is the single reaction time. The numbers are typical, not certified: the standards give how to measure the reaction time and the stopping distance of a particular robot, and the speed of a person depends on the edition. ISO/TS 15066 also has power and force limiting, where contact is allowed below limits for each part of the body; that is a different method and is not modelled here.

## What Breaks First

- **A protective distance with the person taken as still.** See `E-ARM-0091`.
- **A speed limit that does not fall as the person gets close.** See `E-ARM-0092`.
- **A reaction time taken from the sheet and not measured.** See `E-ARM-0093`.

## Ship It

`rc/arm/ssm.hpp` holds `SsmParams`, `protective_distance` and `max_speed`.
