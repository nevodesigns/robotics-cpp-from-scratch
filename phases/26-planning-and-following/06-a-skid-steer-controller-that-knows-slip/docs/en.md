# A Skid Steer Controller That Knows Slip: The Track It Really Turns On, the Speed the Ground Allows in a Turn, and Why the Plain Follower Slides Wide

> A skid steer vehicle turns on a track wider than the one it has, and only as fast as the ground lets it.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 26-04, 23-02, 23-05

## The Problem

A skid steer vehicle has no steering. It turns by driving the wheels on one side faster than the other, and the wheels on a side have to slide sideways to let it. Two things follow that a car does not have. The turn it gets from a difference in wheel speeds is less than the plain geometry says (lesson 23-02: the effective track is `chi` times the real one, with chi from 1 up to 1.5 or more on a floor that grips). And the sliding is the turning, so the ground decides how hard a turn may be (lesson 23-05) at a given speed.

## The Concept

### The wheel speeds, with the track it turns on

A follower (here pure pursuit, 26-04) says what curvature to drive. The wheel speeds that give a speed `v` and a turn rate `w` are

```
sum of the wheel speeds        = 2 v / (r (1 - slip))
difference of the wheel speeds = w chi B / r
```

with `B` the track and `r` the wheel radius. A controller that takes `chi = 1` and `slip = 0` asks for a turn that the body makes only 1/chi as sharply: on tarmac, with the real chi of 1.5 and a slip of 0.05, the controller that knew both held the path to 0.042 m at most, and the one that did not strayed 0.115 m, sliding wide of every bend. See `E-VEH-0097`.

### The speed the ground allows in the turn

The sideways acceleration of a turn is `v squared times the curvature`, and the surface grips only so much, `mu g`. On a straight there is no limit to speak of. In the bend, the controller caps the speed at a margin of 0.7 of the speed at which the tyres let go, and never lets it fall below a floor (a vehicle that stops in every turn is not following anything). At the same curvature the ground allowed 2.12 times the speed on gravel as on ice.

On ice at 1.2 m/s the plain controller let go in 51 steps (a vehicle that has let go goes where its momentum takes it, not where the wheels point) and finished in 4.6 s; the one that limited its speed let go in 0 steps and finished in 5.4 s: slower, and on the path. See `E-VEH-0098`.

### A floor, or the controller stops

On ice the limit in a tight turn is below the floor of 0.2 m/s only for very sharp curves, but a controller that can command zero speed in a turn commands zero speed in the turn: it never gets out of it. See `E-VEH-0099`.

## Build It

`skid_wheel_speeds` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- The sum of the wheel speeds is 2 v / (r (1 - slip)); the difference is w chi B / r; the left is half of (sum - difference), the right half of (sum + difference).

As shipped, 3 of the 5 tests fail, and these are the ones your code has to turn green:

- The wheel speeds make the body go and turn as asked.
- A controller that knows chi and slip holds the path, one that does not strays.
- On ice the plain controller lets go in the turn and one that limits its speed does not.

The other 2 pass already, because they test the parts that are given.

```
rcpp verify 26-06
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/drive/skid_follow.hpp`.

```cpp
rc::drive::SkidFollowConfig cfg;
cfg.chi = calibrated_chi;     // from the lesson 23-02 and 23-08 measurements
cfg.slip = calibrated_slip;
cfg.limit_speed = true;
const rc::drive::SkidCommand c = rc::drive::skid_follow_command(geometry, cell, cfg, pose, path, idx);
```

**What this does not do.** The chi and the slip are constants, and in life they change with the surface and with the turn (the more it turns, the more it slides). The traction limit is for one cell: the speed of the whole bend uses the cell the vehicle is on, not the cells ahead. The controller does not check whether the motors can give the thrust a pivot needs (23-02: a 10 kg vehicle could not pivot on gravel with its gear motors); it asks and the motors give what they give. The plant is the kinematic model of 23-02, so the "letting go" is counted, not simulated: after it, the model does not show where the vehicle goes. There is no acceleration limit.

## What Breaks First

- **A skid steer controller that uses the plain track.** See `E-VEH-0097`.
- **A turn taken at a speed the ground cannot hold.** See `E-VEH-0098`.
- **No floor on the speed in a turn.** See `E-VEH-0099`.

## Ship It

`rc/drive/skid_follow.hpp` holds `SkidCommand`, `SkidFollowConfig`, `skid_wheel_speeds`, `skid_follow_command`, `SkidRun` and `skid_follow_run`.
