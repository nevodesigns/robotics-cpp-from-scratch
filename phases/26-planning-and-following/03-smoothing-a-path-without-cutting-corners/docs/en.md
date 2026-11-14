# Smoothing a Path Without Cutting Corners: Pulling Points Straight, Holding Them Near the Plan, and Not Into the Wall

> A smoother that does not look at the walls will cut the corner it was asked to round.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 26-01, 24-04

## The Problem

A path from a planner is a chain of points, and the chain has kinks: a corner of 90 degrees where a grid path turned, a wiggle where a search chose between two nearly equal ways. A controller that follows the kinks follows them with its wheels. What is wanted is the same route, with the kinks taken out, and without moving it into anything.

## The Concept

### Pull towards the neighbours, hold near the plan

Put the points an even distance apart (resampling), keep the two ends where they are, and then, again and again, move each interior point by two pulls: one towards the midpoint of its two neighbours, which straightens the chain, and one back towards where the plan had it, which stops the chain from sliding away from the route it was meant to follow. The weights, 0.3 for the first and 0.1 for the second here, decide how smooth and how faithful the result is. The loop stops when nothing moves more than a tolerance.

The curvature of three points in a row (the circle through them, Menger curvature) measures it. A corner of 90 degrees with points 0.2 m apart had a sharpest curvature of 7.1 per metre; after smoothing it was 3.64, in 65 sweeps: about half, and still several times the car's limit of 1.35. See `E-VEH-0088`.

### Faithful or smooth: one knob

A light smoother (the plan pulled hard, the neighbours softly) stayed within 0.04 m of the plan; a heavy one (the opposite) moved up to 0.63 m, and even that left a sharpest curvature of 1.55, still over the car's 1.35. More smoothing means more distance from what the planner checked, and the planner checked the plan, not the result. Rounding a corner of 90 degrees to the car's own radius takes more room than a smoother that holds the plan will give. See `E-VEH-0089`.

### Do not smooth into the wall

A small block just inside a corner is exactly where a smoother wants to go, because the shortcut inside the corner is the straightest chain. With no grid, 5 points ended inside the inflated block. With the grid, a move into a cell that is inscribed or worse is simply refused, and 0 ended there. See `E-VEH-0090`.

## Build It

The update of each point in `smooth_path`, in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- For each interior point, move it by the data weight times (plan - point) plus the smooth weight times (previous + next - 2 point), unless the new place is blocked; add the move to `moved`.

As shipped, 3 of the 5 tests fail, and these are the ones your code has to turn green:

- Smoothing lowers the sharpest curvature of a path with a corner, and the ends stay.
- A smoother that does not look at the walls cuts the corner into one, and one that does cannot.
- A heavy smoother wanders further from the plan than a light one.

The other 2 pass already, because they test the parts that are given.

```
rcpp verify 26-03
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/drive/smooth.hpp`.

```cpp
const auto even = rc::drive::resample(planned, 0.2);
const auto smooth = rc::drive::smooth_path(even, &master_costmap, rc::drive::SmoothConfig()).path;
```

**What this does not do.** It does not promise the curvature will be under the car's limit: it lowers it, and the number above says by how much. A path that must be driven needs a curvature check afterwards, or a planner that already made one (26-01): a corner of 90 degrees from a grid is better left to the planner than rescued by the smoother. The grid check refuses single moves into a blocked cell but does not check the segment between two points, so a thin wall between two free cells can be crossed. The ends are fixed, so a heading at the end is not controlled. Smoothing shrinks a loop. The headings of the points are not set: they follow from the tangent, worked out by whoever uses the path.

## What Breaks First

- **A corner left in the path.** See `E-VEH-0088`.
- **A path smoothed far from what was planned.** See `E-VEH-0089`.
- **A smoother that cuts into the wall.** See `E-VEH-0090`.

## Ship It

`rc/drive/smooth.hpp` holds `SmoothConfig`, `SmoothResult`, `resample`, `cell_blocked`, `smooth_path`, `menger_curvature` and `max_menger_curvature`.
