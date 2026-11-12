# Frames for a Field Robot: Transforms in 2D and 3D, and a Tree That Answers Where a Lidar Point Is on the Map

> A point has no position until you say in which frame, and a robot has at least four frames before it has done anything.

**Type:** Build
**Time:** about 75 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-01, 21-10

## The Problem

A lidar returns a point 4 m ahead of the lidar. The planner wants it on the map. Between the two there is a mount on the robot, the robot's place in the odometry, and the odometry's place in the map: three transforms, and a program that gets any one of them in the wrong order puts the obstacle in the wrong place with total confidence. Robots that drive outdoors add a fourth problem, which is that the world is not flat, so a point that was in the plane now has a height, and a sensor tilted by five degrees is looking at the ground and not the horizon.

## The Concept

### A pose is a transform

A pose says where a frame is relative to another, and so it is also the rule that takes a point written in the child frame and writes it in the parent. In the plane that is `Tf2`, an x, a y and an angle. In space it is `Tf3`, a rotation matrix and a translation, built from roll, pitch and yaw. `compose(a, b)` applies **b first and then a**, and `inverse` undoes one. Composing a transform with its inverse left an error of **0.2 times 1e-15**, which is rounding.

### The order matters

A shift of 1 m along x and a turn of 90 degrees do not commute. The point (1, 0) went to (1, 1) when it was turned and then shifted, and to (0, 2) when it was shifted and then turned: **1.414 m** apart. This is the commonest error in a robot's geometry code, it never crashes, and it is invisible until the robot turns. See `E-FIELD-0004`.

### The tree

A robot has a few frames, each fixed in the one above: `map`, `odom`, `base`, `lidar`. `FrameTree` stores each as a pose in its parent, and `between(from, to)` walks both to the top and joins them. With the odometry offset (0.5, 0.2) and turned 0.1 rad, the base at (3, 1) turned 0.7 rad and the lidar 0.30 m ahead of the base, a point 4 m ahead of the lidar was at **(6.381, 4.579)** on the map, which agreed with the plane arithmetic done by hand to 1e-9. When the base moved 2 m in the odometry frame the answer moved **1.990 m** on the map (2 m, turned by the odometry's 0.1 rad), because `update` changes one link and the rest is recomputed.

It answers **no** when there is nothing to answer: a frame it was never told about, two frames in different trees, a loop. A tree that returns the identity for those has put the robot at the origin and said nothing. See `E-FIELD-0006`.

### A tilted sensor

A lidar mounted 0.25 m above the ground and tipped 5 degrees down saw the ground at a horizontal range of **2.858 m**, which is the height over the tangent of the tilt. Everything the sensor sees beyond that range is above the ground, and everything nearer is a hole in the data. A planner that treats the scan as flat has a floor that it reads as a wall, or the reverse. See `E-FIELD-0005`.

## Build It

`compose` and `inverse` for `Tf3` in `exercise/solution.hpp`.

```
rcpp verify 22-02
```

## Use It

`rc/field/frames.hpp` opens the module `rc::field`.

```cpp
rc::field::FrameTree tree;
tree.add("base", "odom", rc::field::from_rpy(3.0, 1.0, 0.0, 0, 0, 0.7));
tree.add("lidar", "base", rc::field::from_rpy(0.30, 0.0, 0.25, 0, 0.087, 0));
rc::field::Tf3 to_odom;
if (tree.between("lidar", "odom", to_odom)) { /* apply it to each point */ }
```

**What this does not do.** It has no time: a real tree has to answer for a moment in the past, because a scan was taken 50 ms ago and the robot has moved since. It holds rotations as a matrix, not a quaternion, so a long product drifts from orthogonal and nothing re-normalises it. And it does not check that a pose is a rotation, so a matrix filled in by hand is believed.

## What Breaks First

- **Two transforms composed the wrong way round.** See `E-FIELD-0004`.
- **A sensor mount treated as flat.** See `E-FIELD-0005`.
- **A missing frame that gives the identity.** See `E-FIELD-0006`.

## Ship It

`rc/field/frames.hpp` holds `Tf2`, `Tf3`, `Point3`, `from_rpy`, `apply`, `compose`, `inverse` and `FrameTree`.
