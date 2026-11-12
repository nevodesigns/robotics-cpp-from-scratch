# Describing a Robot Once: One Text File With a Source for Every Number, and Two Models Made From It, the Built In One and the SDF

> If a robot's width is written in two places, one of them is already wrong.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-01, 22-03, 22-07, 21-10

## The Problem

A robot's width is in the planner, which keeps clear of walls by it; in the built in simulator, which collides with it; in the controller, which turns by it; and in the world given to another simulator, which draws it. Four places. Change the width in three and the fourth is a robot that is not the one that was built, and nothing says so: each simulator is self consistent, and the results differ by exactly the amount of the difference. The cure is the one this course applies to the scenario: **write it once, in a file, and make everything else from the file.**

The second half of the problem is where the numbers came from. A model with a track width of 0.16 m is a robot, or a guess. The file therefore records, for **every** number, either the **source** it was taken from or the word **`typical`**, which says plainly that it is a value for a robot of this class and not a measurement of this one.

## The Concept

### The file

```
robot burger
kind diff_drive
wheel_radius 0.033 source ROBOTIS TurtleBot3 Burger constants, as quoted in the Webots example
track_width  0.160 source ROBOTIS TurtleBot3 Burger constants, as quoted in the Webots example
length       0.138 source third party guide to the Burger, not checked against the e-Manual
...
wheel_mass   0.03  source typical
max_speed    0.22  source ROBOTIS e-Manual, Burger
```

The reference robot is a small base of the class of the TurtleBot3 Burger, a differential drive robot used in teaching. **Where a figure could be found it carries where it was found**: the wheel radius and separation are in agreement across the sources that were checked, the speed limits are from the ROBOTIS e-Manual, and the body size and the mass of about a kilogram are from a third party guide, which the file says in so many words, because it is not as good a source as the maker's own. **The rest are typical.** The file has **11 numbers and 3 of them are typical**, and a person reading the file can see exactly which. See `E-FIELD-0036`.

A line with a number and no source is **refused**, at its line, as are a key given twice, an unknown key, a bad number, and a key that is missing. The parser is a rule about honesty more than about syntax: a model whose every number can be traced is one whose differences from the real robot can be found.

### Two models

`limits_of` gives the limits the built in model uses (the `VehicleLimits` of lesson 22-03) and `drive_limits_of` the limits of the `FakeDrive` of lesson 22-01: 0.22 m/s and 2.84 rad/s, from the file. `to_sdf` gives the same robot as an **SDF model** for Gazebo Sim: a body, two wheels on revolute joints, a ball caster at the back, the `DiffDrive` system configured with the wheel separation, the wheel radius and the same limits, and an `OdometryPublisher` that publishes the robot's true pose at every step (at twice the step rate, so that no step is skipped) on `/model/<name>/pose_gt`, which is what a test reads in place of a sensor. The test checks that the numbers printed in the SDF are the numbers of the file: `0.16` for the separation, `0.033` for the radius, `0.22` and `-2.84` for the limits, the wheels at plus and minus half the track. See `E-FIELD-0037`.

### Inertia from the shapes

Gazebo needs the inertia of every link, and a robot description with the wrong one is a robot that spins up like a figure skater or turns like a tanker. The file does not ask for them: it works them out from the mass and the shape. A box of 1 kg with sides 0.138, 0.178 and 0.172 m has an `Ixx` of **0.00511 kg m2**, `m/12 * (y*y + z*z)`; a cylinder about its axis has half `m r squared`. The masses are split so that the body, two wheels and the caster add up to the robot: the SDF's masses summed to **1.000 kg** for a robot of 1 kg. A body that took the whole mass and then added wheels on top would weigh more than the robot. See `E-FIELD-0038`.

### Checking the SDF

The test checks that the SDF is well formed, as for the picture of lesson 22-09. A parser of Gazebo's own is a stricter judge: when this lesson was written the output of `to_sdf` was also run through `gz sdf -k` of Gazebo Sim 8.15 (Harmonic) and was accepted. The next lessons run the model in the simulator, which is the real test.

## Build It

The key line of `parse_robot`, and the two inertia formulas, in `exercise/solution.hpp`.

```
rcpp verify 22-12
```

## Use It

`rc/field/robot_file.hpp` opens the module `rc::field`.

```cpp
const rc::field::RobotParse p = rc::field::parse_robot(read_file("burger.robot"));
if (!p.ok()) for (const auto& q : p.problems) report(q.line, q.message);
const std::string sdf = rc::field::to_sdf(p.robot);   // for Gazebo
const rc::field::VehicleLimits limits = rc::field::limits_of(p.robot);   // for the built in model
```

**What this does not do.** Only a differential drive base can be described yet; the other kinds arrive with their tracks. The geometry is boxes, cylinders and a sphere, with no meshes, no sensors in the model (a lidar and an IMU are added in the world of the next lesson), and the friction of the wheels is a fixed 1.0. The inertias assume solid shapes of even density, which a real robot with a battery at one end is not. The sources are strings for a person to read; nothing checks that they are true.

## What Breaks First

- **A number with no source.** See `E-FIELD-0036`.
- **A dimension written in two places.** See `E-FIELD-0037`.
- **An inertia left at the default.** See `E-FIELD-0038`.

## Ship It

`rc/field/robot_file.hpp` holds `Sourced`, `RobotModel`, `RobotParse`, `parse_robot`, `limits_of`, `drive_limits_of`, `Inertia`, `box_inertia`, `cylinder_inertia`, `sphere_inertia`, `sdf_model` and `to_sdf`.
