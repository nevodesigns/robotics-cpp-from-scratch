# Making the Path Shorter and Smoother: Shortcuts Between Random Points, Corners Cut Where There Is Room, and the Clearance Both Quietly Spend

> A planner finds a path; a second pass makes it one that a person would not be embarrassed by, and it must not undo the safety the first one built in.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 30-03, 30-02

## The Problem

A path from RRT-Connect (30-03) is free and bad: it wanders, it has corners where the tree turned, and it was 2.9 times as long as the straight line in joint space. An arm that follows it spends time and torque (29-08) on detours that nothing required. The planner's job was to find a path at all; improving it is a second, cheap pass.

## The Concept

### Shortcuts

Choose two random points of the path (by distance along it, not by vertex, so a long segment is as likely as a short one). If the straight edge between them is free (conservative advancement, 30-02, with the margin the planner used), replace everything between them by that edge. A random path is full of detours that a straight edge would skip, and a hundred or so tries find most of them. A path from RRT-Connect that was 4.83 times the straight line, with 17 points, was 1.85 times after 150 shortcuts, with 17 points (the count did not fall: a shortcut puts in two points of its own and takes out those between). It never gets longer (a replacement is kept only if it is shorter), it keeps its ends, and every edge of it is free. See `E-ARM-0064`.

### Corners

What shortcutting leaves is a polyline with corners at the vertices, and a corner is a sudden change of direction in joint space: a point where the arm is asked to stop and turn. The sharpest was 2.25 rad in the raw path. Cutting a corner replaces a vertex by two points a fraction of the way back along its segment and forward along the next, and joins them by one edge: that edge is checked; the two outer pieces lie on the old segments, which were free, and need no check. Where the check fails the vertex stays. After two rounds the sharpest turn was 0.60 rad, and the path was no longer than before. See `E-ARM-0065`.

### What it spends

Both passes find shorter, smoother paths by going closer to things. A shortcut with no margin is allowed to take the path right up to an obstacle: the smallest clearance of the arm along it was 0.0001 m, against the planner's margin of 0.02 m; with the margin kept in the check it was 0.0202 m. A planner that keeps a margin and a smoother that does not have between them undone what the margin was for. The margin must go through every pass. See `E-ARM-0066`.

## Build It

The two points of `cut_corners` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- The two points are the vertex moved a fraction f towards the previous point and towards the next point.

As shipped, 1 of the 4 tests fail, and these are the ones your code has to turn green:

- Cutting the corners rounds the sharp turns and the path is still free.

The other 3 pass already, because they test the parts that are given.

```
rcpp verify 30-05
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/arm/shortcut.hpp`.

```cpp
auto path = rc::arm::rrt_connect(problem, q_from, q_to).path;
path = rc::arm::shortcut(problem, path, 150, seed, problem.margin);
path = rc::arm::cut_corners(problem, path, 0.25, 2, problem.margin);
```

**What this does not do.** It is still a polyline of straight pieces in joint space, with a small corner at every vertex that is left: the time parametrisation of 30-06 handles corners by slowing at them. It makes the path short in joint distance, which is not the same as short in tool distance, in time, or in torque. The shortcuts are random: the same seed gives the same path, a different seed a different one; a hundred is a number that worked here, not a rule. A real planner may optimise with a cost that includes clearance (so that smoothing is rewarded for staying away from things), which this does not. And a shortcut does not look at joint limits beyond what sampling and the edge checks already did.

## What Breaks First

- **A path left as the planner found it.** See `E-ARM-0064`.
- **A path with corners the arm cannot take at speed.** See `E-ARM-0065`.
- **A smoother that spends the planner's margin.** See `E-ARM-0066`.

## Ship It

`rc/arm/shortcut.hpp` holds `turning`, `max_turning`, `point_at`, `shortcut`, `cut_corners` and `min_clearance`.
