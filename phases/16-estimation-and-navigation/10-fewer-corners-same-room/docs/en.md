# Fewer Corners, Same Room: Pulling a Grid Path Tight Without Cutting a Wall

> A planner finds a legal path. Legal paths are not the same as paths a robot wants to drive.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 16-06, 16-07

## The Problem

A grid planner (lesson 16-06) returns a list of cells, each next to the last. Followed exactly, it is a staircase: forward, diagonal, forward, diagonal, and a pure pursuit controller (lesson 16-04) steers toward each cell in turn. The robot weaves along a path that, in the open, could have been
one straight line.

Most of the cells are not needed. A cell matters only where the path turns to go round something. Between those corners, a straight line does the job, if it really is free.

## The Concept

### Is the line clear

To keep a cell, ask whether the line from the last kept cell to a later one is clear. The line is walked one cell at a time, the way a rasteriser draws it, and every cell it enters must be **free**. Unknown is not free: the line must run only over what has been seen. A line that
leaves the grid is not clear. See `E-NAV-0027`.

### The gap between two corners

Walking cells is not quite enough. A step that moves on both axes at once goes diagonally between two other cells, and if both of those are walls, the line has slipped through a gap that is, for a rover with any width at all, shut.

With walls at (4,5) and (5,4), the diagonal from (3,3) to (6,6) crosses only (4,4) and (5,5), both free, and slips between the walls. A clear line test that looks only at the cells on the line calls it clear. A step on both axes must also have the two cells it passes between free,
and with that the line was called blocked. See `E-NAV-0026`.

### Greedy shortening

From the last kept cell, the anchor, search the path from its end backwards for the furthest cell with a clear line, and keep that one, then repeat from there. The first and last cells are always kept. A path that is already straight comes back as two points.

Planned from (3,3) to (56,36) in a 60 by 40 room with two walls and gaps:

| | points | length (cells) | turns over 10 degrees |
|---|---|---|---|
| as planned | 72 | 92.12 | 7 |
| shortened | 8 | 88.02 | 5 |

Nine tenths of the points were not needed, the path got 4 percent shorter, and two of the turns were the staircase's own. Across sixteen planned paths in the same room none got longer and none lost an end. Each piece of the shortened path was checked by a separate sampler that walks the segment
every tenth of a cell, not by the code under test. See `E-NAV-0027`.

### Counting turns

A turn is a change of heading, and a heading is an angle, so the difference between two has to be taken the short way round. West and then south west is 45 degrees and not 315: `count_turns` takes any difference over pi to be 2 pi minus it. A step that does not move has no heading, and is skipped. See `E-NAV-0028`.

## Build It

`line_is_clear`, `shorten` and `count_turns` in `exercise/solution.hpp`. `cell_is_free` is given.

```
rcpp verify 16-10
```

The room and the planner are deterministic, so the figures above are the same on every run.

## Use It

`rc/nav/shorten.hpp` joins `rc::nav`.

```cpp
const rc::nav::Plan plan = rc::nav::plan_path(grid, start, goal, options);
const std::vector<GridPoint> corners = shorten(grid, plan.cells);
```

**What this does not do.** It shortens by whole cells and keeps the corners where a cell of the path was, so the result is not the shortest path, only a much shorter one, and a path that leans against a wall at the corner stays against it. It checks one cell of width, so a wide rover needs the grid inflated first (lesson 16-07),
or it will have a clear line a body does not fit down. It gives corners and not a smooth curve, which is the pursuit controller's job, and it works on the map as it is now: a line clear on the map can be blocked by what has not been seen.

## What Breaks First

- **A line test that slips between diagonal walls.** A shortcut through a shut gap. See `E-NAV-0026`.
- **A line that crosses unknown cells.** A shortcut through what nobody has seen. See `E-NAV-0027`.
- **A turn of 315 degrees.** Headings differenced without wrapping. See `E-NAV-0028`.

## Ship It

`rc/nav/shorten.hpp` holds `cell_is_free`, `line_is_clear`, `shorten` and `count_turns`.

The path now has the corners it needs and no more.
