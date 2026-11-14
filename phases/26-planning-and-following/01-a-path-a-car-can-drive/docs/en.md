# A Path a Car Can Drive: Searching Over Pose and Heading With Arcs the Car Can Make, and Why the Grid Path With Its Corners Is Not a Path at All

> A planner for a car plans in poses, because a car cannot be anywhere facing any way.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 24-04, 23-03, 21-10

## The Problem

Lesson 24-04 made a grid of costs. The easiest way to plan a path on a grid is to search from cell to cell, which is what A* does, and for a robot that can turn on the spot it is nearly enough. For a car it is not. A car cannot be in a cell facing any way, it can only be in a cell facing the way its last move left it, and it can turn only as sharply as its steering allows (lesson 23-03). A path through the cells of a grid goes straight for a while and then turns a corner of 90 degrees in no distance at all, which asks for a curvature that no car has. It is not a path the car can drive; it is a picture of where it might like to go.

## The Concept

### Search the poses

Hybrid A\* searches over what the car can actually do. A node is a pose (x, y, heading). From it the search tries a few steerings (the largest either way, half of it, and straight) and drives each as an arc of the bicycle model for a fixed length, 0.6 m. The new pose is kept if every point along the arc is on a cell whose cost is below inscribed (lesson 24-04), so the car's body does not touch anything. The result is a chain of arcs, so every part of it is something the car can drive, with a curvature no more than its limit.

Poses are continuous, and a search over continuous poses never ends, so the hybrid is that the nodes are kept one to a cell of a coarse grid of position (0.25 m) and heading (24 headings): a new pose that falls in a cell already reached more cheaply is dropped. The cost of a move is its length, plus a penalty for steering at all, a penalty for changing the steering from the move before (which makes the path smooth), a penalty for reversing, and the cost of the cells it crosses (so it keeps away from walls that are not quite in the way). The heuristic is the straight distance to the goal, which never overestimates.

### The path is driveable, the grid path is not

On an empty field the path from (2, 2) facing east to (12, 8) facing north had a largest curvature of 1.35 per metre, within the car's limit of 1.35. The path of a grid between the same poses, east and then north, has a corner of 90 degrees between two poses 0.1 m apart: a curvature of 15.7, ten times what the car can do. A planner on a grid followed by a smoother is a common way to a drivable path, and the next lessons are about it; this one finds a path that needs none. See `E-VEH-0082`.

### Through a gap

A wall across the field with a gap of 2 m, inflated by the car's radius, was driven through at the gap in 2277 expansions. A goal walled in, with no way to it, was reported not found after 4000 expansions, the limit set, with an empty path: a planner that cannot find a path must say so and not return the nearest. See `E-VEH-0083`.

### The cost of the heading

Keeping 12 headings to a cell is a coarse and fast search; 48 is fine and slow. With 12 the search took 1146 expansions and with 48 2675. More headings give a path that follows the goal heading more closely and costs more time; the right number is the fewest that lets the car reach its goal heading within the tolerance. See `E-VEH-0084`.

## Build It

The search loop of `hybrid_astar` in `exercise/solution.hpp`.

```
rcpp verify 26-01
```

## Use It

`rc/drive/hybrid_astar.hpp` opens the module `rc::drive`.

```cpp
rc::drive::HybridConfig cfg;
const rc::drive::HybridResult r = rc::drive::hybrid_astar(master_costmap, start, goal, cfg);
if (r.found) follow(r.path);
```

**What this does not do.** The heuristic ignores the walls and the car's turning, so a goal behind a wall or a goal behind the car costs many expansions; the usual improvement, the cost of a path through the free cells ignoring the heading, is not here. The arcs are of one length and the steerings five, so the goal heading is reached only to a tolerance, which a Dubins path from the last node (the next lesson) cures. Reversing is a flag with a flat penalty, with no cusp penalty, so a path may reverse more than a person would. The footprint is the inflated centre, a circle. And the path is a chain of arcs with kinks in the curvature at each join, which a car cannot follow exactly: the controller smooths it.

## What Breaks First

- **A path from a planner that does not know the car.** See `E-VEH-0082`.
- **A planner that returns something when there is no path.** See `E-VEH-0083`.
- **A search too fine to finish.** See `E-VEH-0084`.

## Ship It

`rc/drive/hybrid_astar.hpp` holds `CarPose`, `HybridConfig`, `HybridResult`, `HybridNode`, `drive_arc`, `wrap_pi`, `arc_is_free`, `node_key`, `hybrid_astar` and `max_curvature`.
