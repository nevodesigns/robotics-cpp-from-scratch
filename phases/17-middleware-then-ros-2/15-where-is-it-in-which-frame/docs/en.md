# Where Is It, in Which Frame: A Tree of Coordinate Frames and the Transform at the Right Time

> The obstacle is one metre ahead of the laser. The laser is on the robot, the robot is in the room, and the room is on the map. Which of those is the question?

**Type:** Build
**Time:** about 130 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-13, 06-01, 01-06

## The Problem

Every position on a robot is a position **in a frame**. A lidar reports a point one metre ahead of the lidar, which is a point in the laser's frame. The planner wants it in the map's. Between them are the frame the laser is bolted into, the robot's base, the odometry's own origin, and the map: a chain, each link a transform.

ROS 2 keeps these as a **tree**, the `tf` tree, with each frame hanging from one parent, and any node can ask how two frames are related at a given **time**. The time matters: the robot moves, and the laser's scan was taken a moment ago.

Lesson 6-01 had transforms. This lesson is the tree around them, and what a lookup must do about time and about being asked something that was never reported.

## The Concept

### A transform and its inverse

A `Transform2` is a position and a heading: where a frame is, seen from the frame it hangs from. `compose(a, b)` is first b, then a. `inverse(a)` undoes it, so that `compose(inverse(t), t)` is nothing, which the test checked to 1e-12 for a transform of (3, -2, 2.5 rad). Headings are wrapped to within a half turn after every step: 3.0 rad turned by 3.0 rad is 6.0, which is **-0.283**, the same
direction. See `E-BUS-0043`.

### The short way round

When a transform is interpolated between two reports, the heading goes the short way. Halfway between 170 and -170 degrees is 180, and not 0:

```
halfway between 170 and -170 degrees: 180.0 degrees
```

Averaged as plain numbers it would be 0, pointing the opposite way. The blend takes the difference first, wraps it, then goes part of the way. And halfway between -10 and 10 is 0, as expected. See `E-BUS-0043`.

### A tree

Four frames: the map, the odometry frame in it at (10, 5), the robot's base in odometry at (2, 0) facing up, and a laser 30 cm in front of the base. An obstacle 1 m ahead of the laser:

```
map position (12.00, 6.30)
```

The lookup finds the nearest frame the two have in common, works out each as seen from there, and combines them with an inverse: `compose(inverse(target_from_common), source_from_common)`. That is why two frames on different branches of the same parent can be related, as a laser and an arm, both on the base: the arm in the laser's frame came out at (-0.4, 0.2). And the other way works too: the point in the map, looked up from the map to the laser, came back as (1, 0). See `E-BUS-0044`.

### A frame that moves

A frame given at several times is known between them. A base at 0 at time 0 and at 0.6 m at time 1 is found at 0.3 at the midpoint, 0.15 at a quarter. A **static** frame, such as a sensor mount, is the same at any time. See `E-BUS-0044`.

### The right time

A scan arrives half a second after it was taken, and the robot has moved. The robot drives at 0.6 m/s along x, and the scan is stamped at 1.0 s and used at 1.5 s. A post 2 m ahead of the laser:

| transform used | post placed at x |
|---|---|
| the one at the scan's stamp, 1.0 s | 2.60 |
| the latest, 1.5 s | 2.90 |

The error is **0.30 m**, which is the speed times the delay, for every obstacle in the scan. It looks like noise on the map, as all obstacles smear in the direction the robot is moving. Lesson 15-03 made the same point about readings and their stamps. See `E-BUS-0045`.

### What the tree must refuse

Three questions with no answer, and each is a different one:

| asked | answer |
|---|---|
| a frame that is not there | unknown_frame |
| two frames in separate trees | not_connected |
| a time before the first report or after the last | extrapolation |

A lookup that returned something for the last two would give the planner a position that nobody measured. The extrapolation case matters most: a frame reported until 1.0 s and asked about at 1.5 s could be extended at its last speed, and that is a guess about the robot that looks like data. See `E-BUS-0045`.

## Build It

`compose`, `inverse`, `blend`, `lookup` and `at_time` in `exercise/solution.hpp`. `wrap_angle`, `apply`, `set` and `set_static` and the helper functions that walk the tree are given.

```
rcpp verify 17-15
```

Everything is exact geometry with no random numbers, so the figures above are the same on every run.

## Use It

`rc/bus/frames.hpp` joins `rc::bus`.

```cpp
FrameTree tree;
tree.set("base", "odom", t, pose_from_odometry);
tree.set_static("laser", "base", Transform2{0.3, 0.0, 0.0});
Transform2 map_from_laser;
if (tree.lookup("map", "laser", scan_stamp, map_from_laser) == LookupStatus::ok) place_scan(map_from_laser);
```

**What this does not do.** It is two dimensional: a real tree is in three, with rotations that do not commute and that interpolate by quaternion rather than by angle. It keeps every report for ever, where a real buffer keeps the last ten seconds. It has one parent per frame and refuses a loop only by a guard on the walk. It does not wait: a lookup for a time slightly ahead of the latest report is refused, where a real node can wait for the report to arrive.

## What Breaks First

- **A heading averaged as a number.** The wrong way round when the angles straddle a half turn. See `E-BUS-0043`.
- **A chain composed in the wrong order.** Every point in the wrong place by an amount that depends on the pose. See `E-BUS-0044`.
- **The latest transform for a stamped scan.** A smear along the direction of travel. See `E-BUS-0045`.

## Ship It

`rc/bus/frames.hpp` holds `Transform2`, `wrap_angle`, `compose`, `inverse`, `apply`, `blend`, `LookupStatus` and `FrameTree`.

A point can now be said in any frame of the robot, at the time it was true.
