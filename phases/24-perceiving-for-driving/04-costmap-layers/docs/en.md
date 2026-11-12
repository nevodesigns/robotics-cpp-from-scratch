# Costmap Layers: What Is Known to Be There, What Was Just Seen, and What Is Too Close to It, Kept Apart so That Each Can Be Cleared Without Losing the Others

> A map the planner can use is not what the sensors saw, it is what the vehicle's body makes dangerous.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 24-03, 22-07, 21-10

## The Problem

A planner does not want points. It wants to ask one question of any place in the world: **may the centre of the vehicle be here?** The answer is not "is there an obstacle here", because a vehicle has a size, and its centre may not be anywhere an obstacle could touch it. So the map the planner uses is a grid of costs, built in a few steps from the world, and the steps have to be kept apart: what is known to be there from the start, what the sensors have just seen, and what is too close to either, because each of them changes on its own schedule.

## The Concept

### The grid and its values

A `CostGrid` is cells of one byte. The values follow the convention of the ROS costmaps: **0** is free, **253** is *inscribed* (the vehicle's centre in the cell is inside its own radius of an obstacle, so it would be touching it), **254** is *lethal* (an obstacle is in the cell), **255** is unknown, and 1 to 252 is a gradient of "better not". The grid maps a world position to a cell and back, and outside itself everything is unknown.

### Layers

A **static layer** is drawn once from what is known: `rasterise_world` draws the walls and posts of a world as lethal cells. A wall of 4 m was **81 lethal cells** at 0.05 m, and a post of 0.15 m radius **32**, near the area over the cell size.

An **obstacle layer** holds what the sensors see now: `mark_and_clear` marks the cell each point falls in, and, if told to, **clears** the cells along the beam from the sensor to the point. The clearing matters. An obstacle that was seen at 5 m and has gone (a person who walked on) is in a layer that only marks for ever: after the sensor saw a wall beyond it, **7 cells** of ghost were left in the layer that only marks and **0** in the one that cleared, because every beam to the wall passed through the cells where the obstacle had been. A costmap that never forgets fills up with ghosts and closes every door. See `E-VEH-0037`.

Layers combine by taking the **highest** cost cell by cell (`merge_max`), and each can be cleared or redrawn without touching the others.

### Inflation

`inflate` is applied last, to the master, so that it spreads from the lethal cells of every layer. Every cell within the inscribed radius of a lethal one gets 253, and beyond it the cost falls as `252 * exp(-k * (d - inscribed))`, to the inflation radius. With the inscribed radius at 0.25 m and `k` of 3, the cost at a post was **253** inside 0.25 m, **216** at 0.30 m, **119** at 0.50 m and **30** at 1 m, and nothing beyond 1.3 m. The inscribed radius is the vehicle's: it turns the question "does a body of this size fit" into "is the centre's cell below 253", and **the planner can then treat the vehicle as a point**. The gradient beyond it is a preference to keep away from things, not a rule. See `E-VEH-0038`.

### A gap that is too narrow

Two posts 0.60 m apart left **2 free cells** across the line between them for a vehicle with an inscribed radius of 0.25 m, and two posts 0.40 m apart left **0**: the 0.40 m gap is smaller than the vehicle's width of 0.50 m, so the inflation closed it, and it is closed to the planner without the planner knowing the vehicle's width at all. See `E-VEH-0039`.

## Build It

`inflate` in `exercise/solution.hpp`.

```
rcpp verify 24-04
```

## Use It

`rc/perceive/costmap.hpp` opens nothing new in `rc::perceive`.

```cpp
rc::perceive::CostGrid master(spec);
rc::perceive::merge_max_of(static_layer, obstacle_layer);   // master.merge_max(layer) for each
rc::perceive::inflate(master, robot_radius, 1.0, 3.0);
const bool ok = master.at(master.cell_i(x), master.cell_j(y)) < rc::perceive::kCostInscribed;
```

**What this does not do.** The footprint is a circle, and a long vehicle is not: it needs the cost for its heading, or a footprint check along the path. The inflation is the distance to the nearest lethal cell by brute force and is fine for a small grid and slow for a large one. There is no rolling window that follows the vehicle, no unknown space handling beyond the value, and the clearing assumes the sensor is where it says it is. A beam that passes through a transparent obstacle clears it.

## What Breaks First

- **A costmap that never forgets.** See `E-VEH-0037`.
- **A vehicle planned as a point with no inflation.** See `E-VEH-0038`.
- **A gap judged by its width and not the vehicle's.** See `E-VEH-0039`.

## Ship It

`rc/perceive/costmap.hpp` holds the cost values, `GridSpec`, `CostGrid`, `rasterise_world`, `mark_and_clear` and `inflate`.
