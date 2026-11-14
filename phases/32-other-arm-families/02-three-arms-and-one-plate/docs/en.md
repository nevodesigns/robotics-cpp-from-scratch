# Three Arms and One Plate: The Delta Robot, Its Closed Form Both Ways, and Why It Is So Fast

> A delta robot puts the motors on the base and moves only light rods, and it pays with a small, dome shaped reach.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 32-01, 28-04, 29-01, 31-07

## The Problem

32-01's SCARA moves in a plane quickly. Picking small light parts from a conveyor at 100 or more a minute wants more speed than a SCARA, and the weight it still moves is the second link and the motor of the second joint. The delta robot removes that too: all three motors sit on the fixed base and what moves is a light platform held by six thin rods.

## The Concept

### Three arms, one platform

Each motor turns an upper arm (length rf). At its end a parallelogram of two rods (length re) joins it to the platform. A parallelogram keeps the platform parallel to the base whatever the arms do, so the platform has three movements (x, y, z) and no tilt, which suits pick and place from above. Here the base is 0.12 m from centre to motor, the arms are 0.20 m and 0.46 m, and the three motors are 120 degrees apart. See `E-ARM-0106`.

### Inverse kinematics, one arm at a time

Because the platform does not tilt, each arm can be solved on its own. Turn the target into the frame of arm k (rotate by 120 degrees times k). The platform joint is a point at (x0, y0 less rp, z0). The elbow lies on a circle round the motor axis (radius rf, in the plane through the axis) and on the circle where a sphere of radius re round the platform joint cuts that plane (radius the root of re squared less x0 squared). Two circles meet at two points; the one farther out is the elbow. The angle is `atan2`. The sphere test `disc < 0` is the arm saying it cannot reach. See `E-ARM-0107`.

### Forward kinematics, three spheres

The other way is a trilateration: with the three elbows moved in by the platform radius, the platform is a point at distance re from all three, the intersection of three spheres that equal radius gives two points (one above the base, one below), and the lower is taken. A grid of 486 points in a box was asked for and 386 were reachable; the forward kinematics put the platform back at each to 0.000000000 m.

### A dome

The reach is not a box: at a height of -0.40 m the platform could go 0.310 m out from the axis, and less lower down, narrowing toward the bottom. A cell is laid out in the widest part of the dome. The workspace is small for the size of the robot, which is the price. See `E-ARM-0108`.

### Why it is fast

The motors do not move, and the lower rods are light. The moving parts came to 0.78 kg (typical masses, half of each upper arm counted because the rest turns on the motor), against 18 kg for a six axis arm of a similar reach. The same torque gives many times the acceleration.

## Build It

`delta_arm_angle` in `exercise/solution.hpp`.

```
rcpp verify 32-02
```

## Use It

`rc/arm/delta.hpp`.

```cpp
rc::arm::Delta robot;
double th[3];
if (!rc::arm::delta_ik(robot, x, y, z, th)) refuse_the_point();
```

**What this does not do.** It takes the platform as a point that never tilts, and the parallelogram rods as perfect and thin; real rods have ball joints with limits, so the usable angles are smaller than the motor's, and the rods may collide with each other (not checked). There is no fourth axis here (most deltas add a roll through a telescopic shaft). The dimensions are typical and the moving masses are typical. Singularities are not mapped: they are where an arm and its rods are in line, near the edge of the dome.

## What Breaks First

- **A delta asked to work in the whole box it can be seen to cover.** See `E-ARM-0108`.
- **A single IK for the three arms that forgets to turn the target.** See `E-ARM-0107`.
- **Choosing a delta for a task that needs a tilt.** See `E-ARM-0106`.

## Ship It

`rc/arm/delta.hpp` holds `Delta`, `delta_arm_angle`, `delta_ik` and `delta_fk`.
