# Where to Look Next: Frontiers, and the Difference Between Done and Stuck

> A map with no unknown in it is finished. A map with some unknown in it might be finished too, or the rover might be shut in. They look the same from outside.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 16-06, 16-05

## The Problem

Lesson 16-05 gave the rover a map with three answers, free, occupied and unknown, and lesson 16-06 planned through what it knew. A rover dropped in a building has a map that is nearly all unknown, and nobody has told it where to go. It has to choose where to go to see more.

The place to go is where what is known meets what is not: a free cell with an unseen cell beside it. Go there, look, and the map grows. When there is no such place left, exploring is over, or else the rover cannot get to the places that remain.

## The Concept

### A frontier cell

A cell is a **frontier** if it is free and has an unknown cell directly beside it, up, down, left or right. A wall is not a frontier, since it is known. An unknown cell is not either, since nobody can stand in it. On a 30 by 20 map whose left two thirds have been seen, the frontier was the 20 cells of the column
beside the unseen edge. See `E-NAV-0032`.

### Openings

Frontier cells that touch, sideways or diagonally, make one **opening**: a doorway, a corridor mouth, a gap in a wall. A flood fill groups them. The 20 cells above were one opening of 20.

One cell missed inside the seen part, a speck at (3,3), made a frontier of the four free cells round it, a second opening of 4:

```
openings, of sizes 20 4
```

### Where to go

Take the nearest opening, and measure nearest by how far the rover would drive. A search outwards from the rover, one free cell at a time, finds it. The first frontier cell it reaches is the goal.

From (5,5) with a wall between the rover and the unseen part:

| | cell | steps to drive |
|---|---|---|
| nearest in a straight line | (19,5) | 34 |
| chosen | (19,15) | 24 |

The one in a straight line is nearer the rover as a crow flies and past a wall, with a long way round by the gap at the top. The chosen one is the fewest steps to any frontier cell, and the test checks it against its own search. See `E-NAV-0033`.

### The speck

A minimum size for an opening keeps the rover from driving across a room to look at one cell. From (4,4), beside the speck, a minimum of 5 went on to (19,15), the real opening past the wall, and a minimum of 1 stopped at (3,4), right next to the speck. See `E-NAV-0034`.

### Finished, or stuck

Three answers, and the third is the one a careless version loses:

- **found**: there is an opening, and a way to it.
- **finished**: there is no opening of the minimum size. The map is as complete as it will get.
- **unreachable**: there is an opening and no route to it over free cells.

A map with everything seen gave finished. A rover in a sealed room, with a sliver past the wall seen through a window, gave unreachable, and so did a start position inside a wall. A version that says only "no goal" cannot tell a rover that has finished from one that is shut in. See `E-NAV-0034`.

## Build It

`is_frontier`, `find_frontiers`, `cluster_frontiers` and `choose_frontier` in `exercise/solution.hpp`. The `Explore` enum is given.

```
rcpp verify 16-12
```

The maps are built cell by cell in the tests, so the figures above are the same on every run. The distances are checked by a separate search written in the test.

## Use It

`rc/nav/frontier.hpp` joins `rc::nav`.

```cpp
GridPoint goal;
switch (choose_frontier(grid, rover_cell, 5, goal)) {
  case Explore::found: plan_to(goal); break;
  case Explore::finished: report_map_complete(); break;
  case Explore::unreachable: ask_for_help(); break;
}
```

**What this does not do.** It picks by steps and not by what the rover will learn, so a doorway into a huge empty room and a gap behind a pillar look alike, and a real explorer weights openings by how much unknown is behind them. It counts four neighbours for distance and so overestimates by the turns, which is fine for choosing and
not for planning (the planner of lesson 16-06 gives the real cost). It does not know the rover's width, so an opening a body cannot fit through is still an opening (lesson 16-07 inflates the grid for that), and it works on a map that is changing: a frontier may be gone by the time the rover gets there.

## What Breaks First

- **A frontier counted at a wall or in the unknown.** The rover is sent to a place it cannot stand. See `E-NAV-0032`.
- **The nearest by the straight line.** A trek round a wall that the crow flies over. See `E-NAV-0033`.
- **No answer for stuck.** A rover that is shut in reports it has finished. See `E-NAV-0034`.

## Ship It

`rc/nav/frontier.hpp` holds `is_frontier`, `find_frontiers`, `cluster_frontiers`, `Explore` and `choose_frontier`.

The rover now knows where to go next, and when to stop.
