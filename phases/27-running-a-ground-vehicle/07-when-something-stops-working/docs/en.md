# When Something Stops Working: Modes From Full Speed to a Safe Stop, a Second Fault That Costs More, and Coming Back Up One Step at a Time

> A vehicle that has lost something should do less, not nothing and not the same.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 27-06, 25-06, 19-10

## The Problem

A sensor fails in the field, an hour from anybody. A vehicle that carries on exactly as before is driving on a map that is no longer true. A vehicle that stops dead is a vehicle somebody has to walk to, for a fault that a slower vehicle could have lived with. What it should do is less: a step down in what it will try, matched to what it has lost.

## The Concept

### Four modes

Full speed (2 m/s here), reduced (0.8), creep (0.25) and safe stop (0). They are ordered by how much the vehicle gives up, and each says the speed cap and nothing else: the planner, the follower and the protective fields (27-03) already know what to do at a given speed.

### What each loss costs

| lost | mode | why |
|---|---|---|
| the scanner | safe stop | nothing sees obstacles; the stop fields of 27-03 are blind |
| the gyro | creep | the heading comes from the wheels only (25-01 showed what that is worth) |
| the GNSS, the wheel sensors, or the link | reduced | the position is worse, or there is no one to ask for help |
| **any two** of those four | creep | a second fault costs more than the first: the vehicle has lost the thing that would have caught it |

The scanner lost gave mode 3 (safe stop), the gyro 2 (creep), the GNSS 1 (reduced), and the GNSS and the link together 2 (creep). The sigma of the position moves the mode on in steps of its own (0.3, 0.8 and 1.5 m); a sigma that is not a number is a stop. See `E-VEH-0130`.

### Down at once, up one step at a time

A worse requirement is taken at once. A better one is taken only after it has been better for a dwell (10 s), and then one step. This is the hysteresis of 19-08 for modes. A requirement that flipped between reduced and full every 2 s for 120 s changed 60 times raw, and the manager changed the mode 1 time: it went to reduced and stayed there. After the scanner fault cleared and held, the vehicle was back to full speed in 30 s: safe stop, creep, reduced, full, each after a dwell. See `E-VEH-0131`.

### How long can it go without GNSS

Dead reckoning (lesson 25-06) makes the position's uncertainty grow as the square root of the distance. With a sigma of 0.05 m to begin with and the growth the course measured (0.22 m at 10 m for the 95 percent radius, so k squared is about 0.0048 per metre), the sigma reaches 1.5 m after 468 m: `d = (limit squared - sigma0 squared) / k squared`. On the way, the sigma steps the mode down to reduced, creep and finally a stop, so a vehicle that has lost GNSS slows before it is lost. See `E-VEH-0132`.

## Build It

`required_mode` in `exercise/solution.hpp`.

```
rcpp verify 27-07
```

## Use It

`rc/drive/degraded.hpp`.

```cpp
rc::drive::ModeManager manager(policy);
const rc::drive::Mode mode = manager.update(now, rc::drive::required_mode(health, policy));
speed = std::min(speed, rc::drive::mode_speed_cap(mode, policy));
```

**What this does not do.** The table of losses is one vehicle's judgement: a vehicle that navigates by GNSS alone, or one that is never teleoperated, would rank them differently. It has no knowledge of why a sensor is reported lost (a flag), and a sensor that is wrong but reports itself well is not a fault here (the cross checks of 19-07 are for that). The cap is on speed only: it does not change the fields, the planner's margins or the path, which a fuller design would (a wider berth in a reduced mode). The growth of the sigma is a model, with one constant, and it does not know about terrain. The dwell is one number for every step. Nothing tells the operator; a mode change must be logged and reported (27-05, 27-06).

## What Breaks First

- **All or nothing.** See `E-VEH-0130`.
- **A mode that flaps.** See `E-VEH-0131`.
- **Dead reckoning with no limit.** See `E-VEH-0132`.

## Ship It

`rc/drive/degraded.hpp` holds `Mode`, `VehicleHealth`, `ModePolicy`, `mode_speed_cap`, `required_mode`, `ModeManager` and `dead_reckoning_distance`.
