# Slopes and Steps: What Grip, Tipping and the Legs' Reach Each Allow, How High a Foot Can Be Put, and How High the Servos Can Swing It

> A slope is limited by the first of three things to give out, and a step by the geometry of the leg and then by the speed of its servos.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 35-07, 34-05, 34-01, 23-05

## The Problem

A quadruped that can walk on a floor is sent to a ramp, a hill, stairs, a kerb. Four different things can say no, in different places, and the robot should know which will give first for the terrain it faces. For a slope they are grip (the feet slip), tipping (the body goes over) and reach (the legs cannot stretch). For a step the foot can be put only so high, by the geometry of the leg and then by the speed of its servos.

## The Concept

### Three limits on a slope

On a slope of angle a: the feet slip when tan a exceeds the friction (the friction 0.6 gave 31.0 degrees; wet at 0.3, only 16.7 degrees); the body tips when the weight's line leaves the downhill feet, atan of the half wheelbase over the height of the centre of mass (35.1 degrees for 0.27 m, the formula of 19-09 along the body); and a body kept level needs the uphill legs to reach up and the downhill legs down by the half wheelbase times the slope, from the legs' spare length (0.10 m up and 0.12 m down): 27.8 degrees. The lowest of the three is the first to give: number 2 here (0 friction, 1 tipping, 2 reach), the reach. If the body tilts to follow the slope (the attitude of 35-04 set to the slope's), the reach limit disappears and the others stay. See `E-LEG-0045`.

### How high a step

By geometry first: a foot put 0.10 m ahead of the hip, with the body 0.27 m up, can be raised by the largest rise for which the inverse kinematics of 34-01 has a solution with the knee inside its limit (the knee folds as the foot comes up under the hip). It came to 0.229 m: far more than a servo can swing, and it grows the further ahead the foot is put (0.168 m straight under the hip, where the knee folds most), until the foot is at hip height. A tighter knee limit lowers it. See `E-LEG-0046`.

### Then the servos

The swing has to lift the foot over the step and onto it in the swing time, and 34-05's joint rates say how fast the servos must turn. With a swing of 0.20 s and servos that give 8 rad/s (typical) the highest step was 0.033 m, a fraction of the geometric limit: the servos, not the legs, set how high a step the robot can climb at that pace, and a longer swing allows more (the test checks). A robot that must climb stairs has to slow down to do it. See `E-LEG-0047`.

## Build It

`max_step_height` in `exercise/solution.hpp`.

```
rcpp verify 35-08
```

## Use It

`rc/legged/terrain_limits.hpp`.

```cpp
const double a_max = std::fmin(rc::legged::max_slope_friction(mu), rc::legged::max_slope_tip(half_wheelbase, com_h));
const double s_max = rc::legged::max_step_servo(leg, x_land, stand_height, swing_time, 0.03, 8.0);
```

**What this does not do.** The slope is straight and plane, the centre of mass is at a given height (it moves with the legs, 33-05) and friction is a single number (23-05). The step is a rise at a place that is known (stairs are many steps, with a front edge to clear and where the foot lands on the tread to decide, 36-03). The body's pitch and height during a step are not changed to help. The servo rate is one number (29-07); real servos also have torque limits (34-06). The numbers are typical of a 12 kg robot.

## What Breaks First

- **A slope limit from friction alone.** See `E-LEG-0045`.
- **A step height from the geometry alone.** See `E-LEG-0047`.
- **A knee limit not counted.** See `E-LEG-0046`.

## Ship It

`rc/legged/terrain_limits.hpp` holds `max_slope_friction`, `max_slope_tip`, `max_slope_level`, `max_step_height` and `max_step_servo`.
