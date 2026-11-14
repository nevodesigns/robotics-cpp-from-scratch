# One Yardstick for Five Robots: Reach Volume by Sampling, the Room a Robot Takes for It, and What the Numbers Do and Do Not Say

> To compare robots that are built differently, ask each the same question: can you put the tool here, pointing down?

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 32-04, 32-03, 32-02, 32-01, 28-08

## The Problem

Phase 32 has put four new kinds of robot beside the six axis arm: the SCARA, the delta, the Stewart platform and the gantry. Each one's maker will say it is the right one. To choose, the things that matter have to be put on one footing: how much space can each one work in, how much room does it take for that, and what does the number leave out. This lesson builds the yardstick, and says plainly what it cannot measure.

## The Concept

### One question for all of them

Each robot is built differently, so its reach is described differently (joint ranges, leg strokes, travel). The question that all can answer is the same: can you put the tool at this point, level or pointing down? `ReachTest::reaches(x, y, z)` is that question. Each kind of robot has an implementation, and each uses its own kinematics: the SCARA's two elbows (32-01), the delta's three arms (32-02), the Stewart platform's six leg lengths within their stroke at a level pose (32-03), the gantry's box of travel (32-04) and the six axis arm's inverse kinematics with the tool pointing down (28-08). A new kind of robot needs only a new implementation. See `E-ARM-0115`.

### Volume by asking

The reach is an odd shape (a dome, an annulus, a lump) and its volume has no formula. Sampling finds it: ask the question at 40000 random points in a box that holds the reach, and the volume is the box's volume times the share that said yes. The standard error is the box's volume times the square root of p (1 less p) over n, and it shrinks as the square root of the number of points: sixteen times the points gave a quarter of the error in the test with a ball (whose volume is known). For the SCARA the estimate was good to about 0.4 percent. See `E-ARM-0116`.

### The volumes

The reach volumes came out at 0.084 m3 for the SCARA, 0.072 for the delta, 0.006 for the Stewart platform (level, within the leg stroke of the test), 0.035 for the gantry and 1.731 for the six axis arm. The arm's is twenty times the SCARA's, and it would be a mistake to read that as "twenty times better": the SCARA works in a thin slab (its slide stroke is 0.15 m) because its job is a flat table, and the arm's volume is a ball of which most is of no use to a table job.

### The room it takes

A robot that reaches a lot in a lot of room is not efficient. Against the room each one takes (the cylinder, box or sphere that encloses what moves, typical figures that the lesson states and a data sheet replaces) the shares were 0.33 for the SCARA, 0.47 for the delta, 0.07 for the Stewart platform, 0.29 for the gantry and 0.98 for the six axis arm. The arm's share is near one because its room is the sphere it sweeps; the Stewart platform, whose legs and base are large for a small stroke, uses a few percent. These are not a ranking (the room is a choice of envelope), but they say where the floor space goes. See `E-ARM-0117`.

## Build It

`workspace_volume` in `exercise/solution.hpp`.

```
rcpp verify 32-05
```

## Use It

`rc/arm/compare.hpp`.

```cpp
rc::arm::DeltaReach delta(robot);
double se = 0.0;
const double v = rc::arm::workspace_volume(delta, box, 40000, 1, &se);
```

**What this does not do.** Volume says nothing about speed, payload, stiffness or accuracy, which for the families above differ by large factors (32-02's moving mass, 32-04's sag, 31-07's cycle time). The orientation is held level or pointing down, so what the Stewart platform and the arm can do with a tilted tool is not counted. The singular and awkward places inside the reach (32-01, 32-03) count as reachable. The room is typical and a matter of how the envelope is drawn. The box has to hold the reach, and nothing here checks that it does.

## What Breaks First

- **A reach quoted without its orientation.** See `E-ARM-0115`.
- **A volume estimated with too few points.** See `E-ARM-0116`.
- **Choosing by volume.** See `E-ARM-0117`.

## Ship It

`rc/arm/compare.hpp` holds `ReachBox`, `ReachTest`, `workspace_volume`, `ScaraReach`, `DeltaReach`, `StewartReach`, `GantryReach`, `ArmReach` and `room_ratio`.
