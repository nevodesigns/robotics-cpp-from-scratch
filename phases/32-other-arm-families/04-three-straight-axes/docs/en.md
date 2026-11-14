# Three Straight Axes: The Gantry, the Time of a Straight Line Across Axes of Different Strength, and What a Long Beam Does to Accuracy

> A gantry is the simplest robot to point and the hardest to keep stiff, and its stiffness is a beam's.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 32-03, 30-06, 29-08, 31-07

## The Problem

The simplest robot to point is the gantry: three straight axes at right angles, x, y and z, so that the tool's place is the three axes' positions, with no inverse kinematics at all (what 28-04 took a whole lesson for is the identity here). Printers, routers, pick and place over a large bed and warehouse cranes are gantries. The price is not in the control; it is in the stiffness of a long beam, in the speed of three unequal axes, and in the floor space.

## The Concept

### A move across three axes

Each axis has its own speed and acceleration (typical: belts on the long axes give speed and a modest acceleration; a screw on z gives high acceleration and a low speed). If each axis goes on its own, the move is over when the slowest is: a move of 0.4 m in x and 0.4 m in z took 0.825 s, but the tool went by a curved path. If the tool is to go in a straight line (to keep clear of a wall, or to cut or glue), the path speed is limited by each axis's share of the direction: along a unit vector u an axis can give at most v_i over |u_i|. The smallest of those sets the speed, and the same for the acceleration, and here z sets the speed and x the acceleration, so the line took 0.900 s, longer than the axes alone. When one axis sets both limits (a move mostly in x, a little in z) the line costs nothing extra, and with equal axes the two times are equal; the test checks both. See `E-ARM-0112`.

### The beam is the weak part

A gantry's crossbeam is a beam supported at its ends and loaded in the middle: its sag is F L cubed over 48 E I. A beam of 1.0 m (aluminium, an extruded section: typical numbers, and the lesson says so) under 50 N sagged 50 micrometres, a stiffness of 1.0 MN/m, which is a lot less stiff than it sounds for a machine tool. Twice the span made the sag 8 times as great: the cube. That is why a bigger machine is much less accurate unless its beams grow in section, not only in length. See `E-ARM-0113`.

### Resonance

With the moving mass on that stiffness, the first resonance is `sqrt(k over m) over 2 pi`: 71 Hz for 5 kg. A motion that has frequencies near it rings (29-05's computed torque does not help); the rule of thumb is to keep the trajectory's content (jerk limits, 30-06) well below it, and a heavier head lowers it.

### Driven from both ends

A wide beam is driven from both ends, and if the two ends are out of step (one motor behind the other) the beam turns (racks). Ends 0.2 mm out on a span of 1.0 m put 60 micrometres of error at a lever of 0.3 m: the angle times the lever, as in 31-03's tilt. See `E-ARM-0114`.

## Build It

`straight_line_time` in `exercise/solution.hpp`.

```
rcpp verify 32-04
```

## Use It

`rc/arm/gantry.hpp`.

```cpp
rc::arm::Gantry machine;
const double t = rc::arm::straight_line_time(machine, d);
```

**What this does not do.** Each axis is a rigid, exact slide: backlash, belt stretch, and the coupling between axes (the inertia of one moving the others) are not here. The beam is one simple beam with a load at its middle; a real gantry's head moves along it, so the worst place changes, and the sag is the deflection at the load only. The mass is one number and the resonance one mode; real machines have many. The material numbers are typical, not from one maker. Thermal growth is not modelled.

## What Breaks First

- **A straight line planned as if the axes were equal.** See `E-ARM-0112`.
- **A beam that grew in length and not in section.** See `E-ARM-0113`.
- **A two sided drive with no check on the ends.** See `E-ARM-0114`.

## Ship It

`rc/arm/gantry.hpp` holds `Gantry`, `axes_time`, `straight_line_time`, `beam_sag`, `beam_stiffness`, `first_mode_hz` and `rack_error`.
