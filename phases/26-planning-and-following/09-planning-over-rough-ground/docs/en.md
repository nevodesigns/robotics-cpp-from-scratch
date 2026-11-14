# Planning Over Rough Ground: Paying for Sand and Slope in Energy, Paying for Risk, and When the Shortest Way Is Not the Cheapest

> Off the road, the cost of a metre depends on what is under it and which way you are going.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 26-01, 24-05, 23-05

## The Problem

Phase 24 told the planner which cells it could not drive. A cell is not either blocked or free. Sand is drivable and costs far more to cross than tarmac. A slope is drivable and costs far more going up than going across. A patch of loose gravel or of rocks is drivable and risky. If the planner can only see blocked and free it picks the shortest path, and the shortest path is often the wrong one: it ploughs through the sand because it is straight.

## The Concept

### What a metre costs

The work a vehicle does for each metre over a cell is m g (crr cos a + sin a): the rolling resistance of the surface plus the slope along the way it is going (`a` positive uphill). Downhill gives nothing back (there is no regeneration here). For a 10 kg vehicle a metre of sand cost 9.8 J and a metre of tarmac 1.47 J. A metre up a slope of 14 degrees on tarmac cost 25.2 J; across it, 1.47 J. The cost depends on the heading, so the planner has to price a step, not a cell. See `E-VEH-0106`.

### Price a step: distance, energy, risk

A step from a cell to a neighbour costs distance times (1 + w_energy times energy per metre + w_risk times risk), where the risk is the cost of the cell in a cost layer (the traversability layer of 24-05, 0 to 255). A step into a blocked cell (253 or more) is not allowed at all. The weights say what the mission cares about. With no weights it is the shortest path.

### The shortest is not the cheapest

A band of sand 8 m wide across a field of tarmac, with a ford of gravel in it. The shortest path went straight through the sand: 16.0 m, 90 J. With energy weighed as much as distance, the planner went round by the ford: 31.2 m, 66 J: longer, and cheaper. See `E-VEH-0107`.

Risk works the same way. A patch of cost 200 (risky, not blocked) with a way round: with no weight on risk the path crossed it, at a worst cell cost of 200; with a weight, it went round, at 0. See `E-VEH-0108`.

### Not found, and say so

A goal in a ring of blocked cells was not found after 6476 expansions and gave an empty path: as in 26-01, a planner that cannot reach the goal must say so.

## Build It

`step_cost` in `exercise/solution.hpp`.

```
rcpp verify 26-09
```

## Use It

`rc/drive/offroad.hpp`.

```cpp
rc::drive::OffroadWeights w;  w.mass = 12.0;  w.energy = 1.0;  w.risk = 5.0;
const rc::drive::OffroadPath p = rc::drive::plan_offroad(terrain, &traversability, sx, sy, gx, gy, w);
```

**What this does not do.** It plans positions, not poses: the path has the corners of a grid and is for a vehicle that can turn sharply (a skid steer or a differential drive), or for a car after 26-01's search takes it over. The energy is the mechanical work, with no efficiency of the motors, no cost of turning, no speed (a vehicle that goes slower over sand takes longer, and time is not in the price). The surface and the slope per cell are given: nothing here measures them. The weights are chosen by hand, and the right ones depend on the mission: a robot with a full battery and a deadline weighs them differently from one on its way home. A slope seen from the side is measured with the heading of the step, so the grid's eight directions are all the headings it knows.

## What Breaks First

- **A cost that does not depend on the heading.** See `E-VEH-0106`.
- **The shortest path over bad ground.** See `E-VEH-0107`.
- **A risk that is only blocked or free.** See `E-VEH-0108`.

## Ship It

`rc/drive/offroad.hpp` holds `TerrainGrid`, `OffroadWeights`, `energy_per_metre`, `step_cost`, `OffroadPath`, `plan_offroad`, `path_energy` and `path_max_risk`.
