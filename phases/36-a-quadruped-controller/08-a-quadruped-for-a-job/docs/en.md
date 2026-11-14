# A Quadruped for a Job: Every Check of Phases 34 to 36 Run Against One Walk, and the First That Fails Named

> A walk is released when the legs can step it, the ground allows it, a push can be answered, the knees stay cool, the battery lasts and the people near it are safe.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 36-07, 36-06, 36-03, 36-01, 35-08, 35-07, 33-10

## The Problem

Phases 34 to 36 have answered, one by one, what limits a quadruped: the legs, the ground, a push, the knees' heat, the battery, the people near it. A walk (a speed, over a route, with a load, over some ground, among some people) needs all of them to be yes at once, and the first question anyone asks is: can it do this, and if not, which part says no? This capstone runs each check against one walk and names the first that fails, in the manner of 33-10's mobile manipulator and 32-06's arm.

## The Concept

### A walk and a robot, as numbers

A walk: the speed wanted, the steepest slope and highest step on the way, the greatest push it may meet, the extra load, the length of the route, the nearest a person comes. A robot: the leg and the knee motor, the body's mass and the height of its centre, the floor's friction, the leg's spare reach, the swing time and step limits, the reach of a catching step, the battery and the person model (31-06). Writing them down is most of the work. See `E-LEG-0072`.

### Six requirements, in order

1. Speed (36-01): the speed against the fastest the legs allow, the longest step over the shortest stance (1.60 m/s). A speed of 2.0 m/s failed at 1.
2. Terrain (35-08): the slope against the least of the three slope limits (27.8 degrees) and the step against what the servos can swing in the swing time (0.050 m). A slope of 40 degrees failed at 2.
3. Push (35-07): the greatest push against the largest that a catching step can answer (0.31 m/s). A push of 0.9 m/s failed at 3.
4. Knees (34-06, 36-03): the knee's settling temperature in a trot with the load (39.5 K against 60 K) and whether the robot with the load can still get up from a crouch (torque and heat). 30 kg of load failed at 4.
5. Battery: the range from the power (eight joints' winding loss and the idle power) against 1.2 times the route (957 m for a walk of 0.6 m/s). A route of 5000 m failed at 5.
6. People (31-06): the speed the nearest person allows (3.48 m/s at 3 m) against a useful minimum. A person 0.3 m from the route failed at 6.

A walk of 0.6 m/s with 1 kg over 500 m passed all six (1, 1 for yes). Only the first failure is reported, and the figures of the rest are still computed and kept, so they show how far each is from passing. See `E-LEG-0073`.

### Why this order

The order is what is cheapest to change last: the speed is the operator's choice; the ground and the push are the job's; the knees, the battery and the people are the robot's and the site's. When the first to fail is a robot limit, no setting will help, and saying which is the point. And a released walk is a screen, not a proof. See `E-LEG-0074`.

## Build It

`check_job` in `exercise/solution.hpp`.

```
rcpp verify 36-08
```

## Use It

`rc/legged/quad_job.hpp`.

```cpp
const rc::legged::JobVerdict v = rc::legged::check_job(robot, walk);
if (!v.released) report_the_first_failure(v.first_fail);
```

**What this does not do.** Each check is the simple model of its lesson with typical numbers, so this is a screen that finds walks that cannot work, not a proof that a walk that passes will. The mission (the state machine of 36-02), the safety layer (36-06) and the gait itself are not run: nothing here walks. The route is a length and a speed, the ground one slope and one step, the people one distance. The battery model is the winding loss of eight joints and an idle draw (a mechanical and electronic loss model is not here). A real release needs the trial on the real robot, with its delay, heat and stopping measured (36-07).

## What Breaks First

- **A walk released on the top speed alone.** See `E-LEG-0072`.
- **The first failure hidden in a long list.** See `E-LEG-0073`.
- **A robot limit reported as a setting.** See `E-LEG-0074`.

## Ship It

`rc/legged/quad_job.hpp` holds `Job`, `Robot`, `JobVerdict` and `check_job`. With it phase 36 is complete, and so is the legged track (phases 34 to 36): a quadruped that does what it is told and keeps itself safe, with a velocity command turned into a gait, a state machine that stands, walks, sits and gets up, feet that adapt to the ground, a short horizon controller for the body, a safety layer that can take the power away, an honest account of what changes on the real robot, and a walk that is checked before it is released.
