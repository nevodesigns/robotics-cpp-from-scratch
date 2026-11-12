# Can It Drive There: Slope and Roughness From a Height Map, Judged Against What This Vehicle's Motors, Tyres and Wheels Can Do

> Whether ground is passable is not a property of the ground, it is a property of the ground and the vehicle.

**Type:** Build
**Time:** about 85 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 24-04, 23-06, 22-05, 21-10

## The Problem

Lesson 24-04 made a map of where things are. Ground is not a thing, and a map of obstacles says nothing about it. A flat floor, a ramp, a stair and a field of rubble are all "no obstacle" to a lidar that looks across them. Whether the vehicle can drive there depends on two properties of the ground that a height map gives, the **slope** and the **roughness**, and on three properties of the vehicle that earlier lessons worked out: how steep its motors can climb (23-06), how steep its tyres will hold on this surface (22-05), and how steep it can lean before it tips over. The same patch of ground is a road for one vehicle and a wall for another.

## The Concept

### Slope and roughness from heights

A `HeightGrid` holds the height of the ground in each cell. `slope_and_roughness` fits a plane to the 3 by 3 cells round a cell by least squares: the plane's slope is the slope of the ground there, and the spread of the heights about the plane is its roughness. The two are different things: a smooth ramp is steep and not rough, and a field of stones is rough and not steep. A ramp of 10 degrees was read as **10.0 degrees**, with a roughness of nothing, and read the same when it was turned to run along the other axis. Noise of 3 cm on a flat patch gave a slope of nearly nothing and a mean roughness of **0.028 m**. The step is the hard one. A step of 5 cm across cells of 10 cm is, to the plane through the 3 by 3 cells, a slope of **14.0 degrees** with a roughness of **0.0144 m**: inside the limits of the vehicle, both of them, because the plane smooths it away. The steepest rise between two neighbouring cells, **26.6 degrees**, does not smooth, and is over the slope limit. `max_gradient` is that, and the cost uses it. Cells at the edge of the grid, or with an unknown neighbour, are unknown.

### The limits are the vehicle's

`vehicle_limits` is the steepest slope the vehicle can take, which is the **smallest of three**: what the motors can start on (`max_climb`, lesson 23-06), what the tyres hold on this surface (`max_climb_angle`, lesson 22-05), and where the vehicle tips over sideways, `atan(half the track over the height of the centre of mass)`; and the roughest it can cross, **half the wheel radius**. For the reference vehicle on gravel the motors could climb **25.6 degrees** and the gravel held **22.3**, so the limit was **22.3 degrees** and the roughness limit **0.025 m**. A taller vehicle with the same track tips at a smaller slope than it slips, and the limit is then its balance. See `E-VEH-0040`.

### From limits to costs

`traversability_layer` gives each cell a cost from the larger of the two shares of the limits: **0 for flat smooth ground**, rising with the **square** of the share to 252 (so that it is nearly free for most of the range and steep near the limit), and lethal at or beyond a limit. On the test ground the flat floor was free, the ramp of 10 degrees cost something, the slope of 30 degrees was lethal and so was the step, and **2 cells** a row were lethal at the step. It is a layer, like the others of lesson 24-04, and the master takes the highest of them. The cost takes the largest of the plane's slope, the steepest rise to a neighbour and the roughness, each as a share of its limit.

### The same slope, two floors

A ramp of 24 degrees is within what the motors can climb. On tarmac, whose tyres hold more than that, it cost **198**, so the vehicle can take it. On gravel, which holds 22.3 degrees, it cost **254**, lethal. It is the same ground, and the same vehicle, and a different floor, and a map built without the surface would be wrong for one of them. See `E-VEH-0041`.

## Build It

`slope_and_roughness` in `exercise/solution.hpp`.

```
rcpp verify 24-05
```

## Use It

`rc/perceive/traversability.hpp` opens nothing new in `rc::perceive`.

```cpp
const rc::perceive::VehicleLimits lim = rc::perceive::vehicle_limits(drive_train, surface, track, cg_height);
const rc::perceive::CostGrid layer = rc::perceive::traversability_layer(height_grid, lim);
master.merge_max(layer);
```

**What this does not do.** It is a slope in any direction against one limit, where a vehicle can climb more than it can cross: a slope across its path tips it earlier than one along it, and the layer has no heading. A step is judged by the steepest rise between neighbouring cells, which depends on the size of the cell: a step of 5 cm is 27 degrees across cells of 10 cm and 6 degrees across cells of 50 cm, and at the larger size it is gone. The surface is a single one for the whole map. The height map is taken as given; building it from lidar points is the ground estimation of lesson 24-02 per cell.

## What Breaks First

- **A limit taken from the motors alone.** See `E-VEH-0040`.
- **A map made for one surface.** See `E-VEH-0041`.
- **A step missed by a smooth slope test.** See `E-VEH-0042`.

## Ship It

`rc/perceive/traversability.hpp` holds `HeightGrid`, `SlopeRoughness`, `slope_and_roughness`, `VehicleLimits`, `vehicle_limits`, `traversability_cost` and `traversability_layer`.
