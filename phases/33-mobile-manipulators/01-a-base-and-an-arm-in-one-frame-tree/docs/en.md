# A Base and an Arm in One Frame Tree: World, Base, Mount, Tool, and How an Error in the Base Becomes an Error at the Hand

> Put the arm on a base and every number the base gets wrong is multiplied by the arm's reach.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 28-02, 28-01, 22-02, 32-06

## The Problem

Phases 28 to 32 put an arm on a table: its base frame was fixed to the world. Put the same arm on a wheeled base and the base frame moves, so every number the arm uses has to be taken through the base's pose. A fetch task is "the object is on that shelf, in the world"; the arm's inverse kinematics wants it "in the arm's base frame". The first lesson of the mobile manipulator is the tree of frames that joins them, and the point it makes is that the base's errors become the hand's.

## The Concept

### World, base, mount, arm, tool

On flat ground the base's pose is (x, y, heading). The arm is mounted on the base with a fixed offset: some distance forward, a height, perhaps a turn (the mount). The arm has its own table and tool (28-01, 28-02). The tool's pose in the world is the base pose composed with the mount, as the base of the arm's forward kinematics. With the base at (2.0, 1.0) turned 90 degrees and the arm at zero, the tool was at (2.000, 1.550, 0.050) in the world: the arm's own tool place, turned with the base and carried with it, at the same height. See `E-MOB-0001`.

### Into the arm's frame

The inverse kinematics of 28-04 takes a target in the arm's base frame. A target given in the world (a shelf, an object seen by a camera that is localised in the world, 25) is put into it by the inverse of the arm base's pose in the world. The round trip world, arm, world gave the point back to 0.000000000 m. A target straight ahead of the mount is on the arm's own x axis. Forgetting this step is a mistake that gives a large, steady error. See `E-MOB-0002`.

### The base's error is multiplied

A position error in the base is carried over one for one to the hand: 5 mm gave 5 mm. A heading error is multiplied by the distance from the base's centre to the hand: 1 degree moved a hand 0.9 m out 15.7 mm. That is the same lever as 31-03's tilt and 32-04's racking, and it is why a mobile manipulator needs its heading known to a fraction of a degree to grasp (33-07). See `E-MOB-0003`.

## Build It

`tool_in_world` in `exercise/solution.hpp`.

```
rcpp verify 33-01
```

## Use It

`rc/mobile/mm_frames.hpp`.

```cpp
rc::mobile::MobileManipulator m;  rc::mobile::BasePose base{x, y, heading};
rc::arm::Pose tool = rc::mobile::tool_in_world(m, base, q);
```

**What this does not do.** The base is on flat ground: three of the six numbers of a pose (height, roll, pitch) are zero, which a slope or a bump would break (33-05). The base's pose is exact here; where it comes from (25) and how wrong it is (33-07) is the topic of later lessons. The arm is rigid and perfectly mounted: a mount that sags or is a degree out is an error of the kind this lesson ends on. Time is not here: the base moves while the arm reaches (33-04).

## What Breaks First

- **A world point used as an arm target.** See `E-MOB-0002`.
- **A mount measured to the centimetre and not to the degree.** See `E-MOB-0001`.
- **A heading that is a degree out.** See `E-MOB-0003`.

## Ship It

`rc/mobile/mm_frames.hpp` holds `BasePose`, `MobileManipulator`, `base_pose`, `arm_base_in_world`, `tool_in_world`, `world_to_arm` and `hand_error_from_base`.
