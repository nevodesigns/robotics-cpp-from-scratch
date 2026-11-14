# Ackermann Geometry and the Turning Circle: The Bicycle Model, the Two Steering Angles of a Car, the Smallest Circle It Can Drive and the Speed It Can Drive It At

> A car cannot turn on the spot, and the number that says how much room it needs is one division.

**Type:** Build
**Time:** about 80 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 23-01, 22-03, 22-05, 21-10

## The Problem

A differential drive can turn on the spot. A car cannot: its rear wheels point where it points, so it has to be moving to turn, and the sharpest turn it can make is a circle of some size. That size is the first thing a planner for a car-like robot must know, because a path that asks for a tighter bend than the car can drive is not a path. A car also has a detail that a bicycle does not: its two front wheels cannot both point the same way, and a robot that points them so loses grip and scuffs its tyres. This lesson works both out.

## The Concept

### The bicycle model

For planning, the four wheels of a car are replaced by two, one in the middle of each axle, which is a bicycle. The pose is that of the **rear axle's centre**, and with a speed `v` and a steering angle `delta` it moves by `x' = v cos(theta)`, `y' = v sin(theta)`, `theta' = v tan(delta) / L`, with `L` the wheelbase. The turn rate is proportional to the speed: at rest it turns not at all, which is the sentence "a car cannot turn on the spot" in an equation. `BicycleCar` takes a step with the heading at the middle of the step and holds the steering angle within the limit.

The model is for a car-like vehicle of the size of the small research cars used for autonomous racing, and its **wheelbase of 0.3302 m is taken from the F1TENTH car** (the wheelbase named in the vehicle modelling page of a course wiki for it); the **track of 0.20 m and the largest steering angle of 24 degrees are typical** values, marked so in the header, since no source for them was found.

### The smallest circle

At the largest angle the rear axle's centre drives a circle of radius `L / tan(delta_max)`: **0.742 m**. A car driven at that angle for one circumference at 1 m/s came back to its start within 2 mm and with its heading turned exactly once round. Anything tighter than that circle is a path the car cannot follow, however neatly it is drawn, and a planner for a car that does not know this number will draw it. See `E-VEH-0007`.

### Two wheels, two angles

The inner front wheel (towards the centre of the circle) has a shorter way round than the outer. If both are pointed at the same angle they cannot both roll without sliding, and the tyres scrub. **Ackermann steering** points each at right angles to the line to the centre of the circle, which gives `atan(L / (R - B/2))` for the inner and `atan(L / (R + B/2))` for the outer. On the smallest circle the inner was at **27.2 degrees** and the outer at **21.4**, **5.8 degrees** apart. They obey the Ackermann condition `cot(outer) - cot(inner) = B / L`, which is checked to 1e-12; and on a circle of 20 m the two angles differed by less than a tenth of a degree, which is why the difference is invisible on a road and plain on a small car at full lock. See `E-VEH-0008`.

### The lane a car needs

At the smallest circle the outer front wheel sweeps a circle of **0.904 m**, further out than the outer edge of the car beside it, because the wheel is ahead of the axle. The lane the car needs is the distance from the inner rear wheel to the outer front wheel, and it was **0.062 m wider than the car** itself. A corridor as wide as the car cannot be turned in, and neither can one that is wider by less than that margin, which at the scale of a small car is a few centimetres and at the scale of a truck is a metre. See `E-VEH-0009`.

### How fast round it

Round a circle of radius `R` at speed `v` the car accelerates sideways at `v*v / R`, and the tyres can give at most `mu g`: so `v` is at most `sqrt(mu g R)`. On gravel (friction 0.45) the fastest the car could take its smallest circle was **1.81 m/s**. That is a limit the surface sets and not the motor, and a later lesson of this phase puts it together with the others.

## Build It

`ackermann_angles` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- Each front wheel is at the end of a line at right angles to its own rolling, which meets the rear axle's line at the centre of the circle: the inner wheel is at atan(L / (R - B/2)), the outer at atan(L / (R + B/2)).

As shipped, 1 of the 5 tests fail, and these are the ones your code has to turn green:

- The two front wheels are at different angles, and the difference is what the Ackermann condition says.

The other 4 pass already, because they test the parts that are given.

```
rcpp verify 23-03
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/vehicle/ackermann.hpp` opens nothing new in `rc::vehicle`.

```cpp
rc::vehicle::CarGeometry car;
const double steer = rc::vehicle::curvature_to_steer(car, curvature_of_the_path);
double inner = 0.0, outer = 0.0;
rc::vehicle::ackermann_angles(car, 1.0 / curvature_of_the_path, inner, outer);
```

**What this does not do.** It is a kinematic model: the tyres do not slide in it, so it is right at low speed and wrong in a fast bend, where they do, and the dynamic bicycle model of the track on speed (a later phase) adds that. The steering angle is set at once, with no servo that takes time to move. The steering limit is one number where a real linkage has a different limit for each wheel, and the wheels are taken to point exactly where they are told, with no play in the linkage.

## What Breaks First

- **A path with a bend tighter than the car can drive.** See `E-VEH-0007`.
- **Both front wheels pointed at one angle.** See `E-VEH-0008`.
- **A corridor sized to the car's width.** See `E-VEH-0009`.

## Ship It

`rc/vehicle/ackermann.hpp` holds `CarGeometry`, `BicycleCar`, `turning_radius`, `min_turning_radius`, `ackermann_angles`, `swept_radius_outer`, `curvature_to_steer` and `max_speed_in_turn`.
