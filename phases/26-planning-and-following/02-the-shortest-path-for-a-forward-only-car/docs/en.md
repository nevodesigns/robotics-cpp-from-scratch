# The Shortest Path for a Car That Only Goes Forward: Dubins Paths, an Exact Finish for the Search, and the Loop to a Goal Just Behind You

> Between two poses the shortest road for a car is two arcs and a line, or three arcs, and there are only six to try.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 26-01, 23-03

## The Problem

Lesson 26-01 found a path of arcs, and it ended near the goal, within 0.3 m and 20 degrees, because poses are kept one to a coarse cell and the search cannot land exactly on one pose. A car about to dock, park, or line up with a gate needs to end **exactly** on a pose. And a planner often has a start pose and a goal pose in the open, with nothing in the way, where searching is a waste: what is wanted is the shortest road between them.

## The Concept

### Two arcs and a line

For a car that only goes forward and turns no tighter than a radius `r`, the shortest path between two poses was worked out by Dubins in 1957: it is always made of **three pieces**, each a left arc (L), a right arc (R) or a straight line (S), and only six words are possible: **LSL, RSR, LSR, RSL, RLR and LRL**. Each has a closed form in units of the radius: the straight (or the middle arc) is worked out from the geometry of two circles, then the end arcs from the headings. A word may not exist for a given pair of poses (the circles of an outside line may be too close, for instance), so the method tries all six and keeps the shortest that exists.

### Checked by walking it

A formula that is wrong by a sign gives a path that ends somewhere else. So the test walks the three pieces with exact arcs and compares the end to the goal. Over **200** random pairs of poses the end was at most **0.000000000 m** and **0.000000000 rad** from the goal, and **6** of the six words were the shortest at least once. A goal straight ahead is a straight line of exactly the distance, and no path is ever shorter than the distance. See `E-VEH-0085`.

### An exact finish for the search

After the hybrid search of 26-01 ended **0.073 m** and **2.8 degrees** from the goal, a Dubins path from that last pose to the goal pose finished it exactly, at the car's own radius, with a curvature no more than the car's limit of **1.35 per metre**. It was **4.70 m** long, far more than the gap, because the goal lay a little behind and to the side of the last pose and a car that cannot reverse must loop round to it (the same effect as the goal just behind, below); trying the tail from each of the last nine poses of the search found one of **1.25 m**. This is the "analytic expansion" used in practice: search to get round the walls, then a Dubins tail to land on the goal. (A fuller version tries the tail from many nodes of the search and checks it for collisions; this lesson does not check the tail against the grid, so a tail through a wall is possible.)

### The radius is the car's

A radius smaller than the car's looks like a shorter path. With r = 0.4 m, smaller than the car's 0.742 m, the path had a curvature of **2.50 per metre**, past the limit of **1.35**: a path the car cannot drive. See `E-VEH-0086`.

### A goal just behind

The shortest path for a car that **cannot reverse** to a goal 0.5 m directly behind it is a loop: **10.3 times** as long as the distance. A car that can reverse would back up half a metre. Dubins is the shortest path for forward motion only; the family with reversing (Reeds and Shepp, 1990) has more words and a cusp where the car stops and changes direction, and is not here. See `E-VEH-0087`.

## Build It

`dubins_word`, the six closed forms, in `exercise/solution.hpp`.

```
rcpp verify 26-02
```

## Use It

`rc/drive/dubins.hpp`.

```cpp
const rc::drive::Dubins d = rc::drive::dubins_shortest(last_pose, goal, min_radius);
if (d.ok) follow(rc::drive::dubins_sample(d, 0.05));
```

**What this does not do.** It does not check the path against the grid. It does not reverse. It treats the car as a point with a fixed minimum radius, so a car whose steering is slow, or that wants a margin from its limit, should be given a radius larger than its minimum. The curvature jumps from zero to 1/r where an arc meets a line, which a real steering takes time to follow (the next lesson smooths paths). The six forms are standard but long, and the test checks them by walking, not by proof.

## What Breaks First

- **A path that ends near the goal, not on it.** See `E-VEH-0085`.
- **A radius smaller than the car's.** See `E-VEH-0086`.
- **A loop to a goal just behind.** See `E-VEH-0087`.

## Ship It

`rc/drive/dubins.hpp` holds `DubinsWord`, `Dubins`, `mod2pi`, `arc_pose`, `dubins_word`, `dubins_shortest`, `dubins_piece_curvature` and `dubins_sample`.
