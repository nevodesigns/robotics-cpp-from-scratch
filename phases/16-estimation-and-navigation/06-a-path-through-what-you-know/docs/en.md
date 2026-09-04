# A Path Through What You Know: Search, and What It Costs to Be Wrong

> The search reported 59. The path was 81.4 long. Both numbers were computed
> correctly.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 16-05, 03-03

## The Problem

There is a map now, and a robot that can follow a path. Nothing yet produces a
path.

A* over a grid is the standard answer and it is not difficult: a priority queue,
a cost so far, a guess about the cost remaining. The whole search is about a
hundred lines and none of them is where the trouble is.

The trouble is in three numbers that look like details:

- what a diagonal step costs,
- what the guess about the remaining distance is allowed to say,
- what unknown ground costs.

Get any of them wrong and the search still returns a path, still reports a
number, and still looks correct.

## The Concept

### The cost function is what the search optimises

Every step onto a neighbouring cell is charged something. The search then finds
the cheapest total. If that charge does not describe the world, the answer is
optimal for a world that does not exist.

The classic version of this is charging one for a diagonal step. Measured on a
room with two walls and a start and goal 55 cells apart:

| diagonal step | reported | true length | difference |
|---|---|---|---|
| 1.0 | 59.0000 | 81.3675 | **37.9%** |
| sqrt(2) | 73.0833 | 73.0833 | 0.0% |

Two separate failures there, and the second is worse than the first.

**The number is wrong.** 59 for a route of 81.4. Every arrival estimate built on
it is optimistic, by an amount that depends on how much the route zigzags, so it
is optimistic by a different amount every time.

**The route is wrong.** Since the search is minimising the wrong quantity, the
path it settles on is 11.3 percent longer than the shortest one actually
available. It is not merely mislabelled; a better route existed and was passed
over.

So the planner returns both numbers, and a test asserts they agree:

```cpp
RC_CHECK_NEAR(plan.cost, path_length(plan.cells), 1e-9);
```

They are two computations of the same quantity, one by the search and one from
the cells it handed back. When they stop agreeing, the cost has picked up
something that is not distance. That is often deliberate, and it means the cost
is no longer metres, so stop reporting it as metres.

### The guess must not overestimate

A* is only optimal if the heuristic never claims the remaining distance is more
than it is. The usual way to break that is Manhattan distance on a grid you may
cross diagonally:

```cpp
return std::fabs(goal.x - x) + std::fabs(goal.y - y);   // overestimates
```

A diagonal step covers a cell of x and a cell of y at once, so counting the axes
separately charges twice for the part they share. The correct guess discounts
it:

```cpp
const double shared = std::min(dx, dy);
return (dx + dy) + (std::sqrt(2.0) - 2.0) * shared;
```

On the room above, all three guesses found the identical optimal path, and the
inadmissible one was fastest:

| guess | cost | over optimal | expanded |
|---|---|---|---|
| none | 73.0833 | 0.00% | 2078 |
| octile | 73.0833 | 0.00% | 1255 |
| manhattan | 73.0833 | 0.00% | 1068 |

Which is exactly how this survives. So ask a hundred maps instead:

| guess | suboptimal on | worst overshoot | cells expanded |
|---|---|---|---|
| octile | 0 of 103 | 0% | 32 686 |
| manhattan | **30 of 103** | 3.45% | 25 368 |

Three maps in ten, by a few percent, for a fifth fewer cells looked at.

State both halves plainly. The overestimate really is faster, and it really is
wrong, not rarely and not catastrophically. If that trade is worth taking, take
it on purpose, write down the number you measured, and keep the admissible
search so it can be measured again when the maps change. A deliberate 3 percent
is engineering; an accidental 3 percent is a bug that happens to be small this
week.

**The heuristic and the step cost are one design.** Diagonals at `sqrt(2)` means
octile. Forbid diagonals and Manhattan becomes correct while octile becomes the
one that underestimates, which is safe and slow. Change one and the other is
wrong.

### Unknown ground has no safe default

Lesson 16-05 gave the map a third answer. Here is what the planner does with it.

A corridor surveyed only along the bottom, with an obstacle across the surveyed
part and open unseen ground above:

| unknown is | length | cells crossed that nothing has seen |
|---|---|---|
| blocked | 57.49 | 0 |
| expensive, x4 | 57.49 | 0 |
| expensive, x1.05 | 55.00 | 25 |
| free | 55.00 | 25 |

Unexplored space is not merely permitted when it costs nothing, it is
**preferred**: there are no obstacles in it, because nothing has looked.

And `expensive` is a dial rather than a third opinion. Sweeping the multiplier
on that same map:

| multiplier | length | unseen cells |
|---|---|---|
| 1.00 | 55.00 | 25 |
| 1.05 | 55.00 | 25 |
| 1.10 | 57.49 | 0 |
| 2.00 | 57.49 | 0 |

It turns over between 1.05 and 1.10, and everything above that is the same
answer as blocked. The multiplier is where the decision actually lives, and it
deserves a number somebody argued about on a map somebody swept.

- **blocked** for anything carrying a load, moving near people, or not
  recoverable by hand. It is the right default and it is never surprising.
- **expensive** where exploring is the job.
- **free** essentially nowhere.

### No route is an answer

A search that fails has to say so, and the caller has to handle it. An empty
path returned as though it were a valid one becomes "already there" somewhere
downstream, which is a robot that does nothing and reports success.

```cpp
if (!plan.found) hold();
```

The planner also reports how many cells it expanded, which separates "there is
no route" from "it gave up". And a start or goal outside the map is refused
rather than indexed with, because a planner is exactly where an out-of-range
goal arrives from somewhere else.

## Build It

Implement `octile_distance`, `step_cost` and `path_length` in
`exercise/solution.hpp`. The search itself is written for you.

```
rcpp verify 16-06
```

The suite checks the heuristic against the movement model, plans through a room
with two walls, plans it again with the cheap diagonal, sweeps three guesses
over a hundred cluttered maps, and asks a half-surveyed corridor what unknown
ground is worth.

## Use It

**Assert that cost equals length**, and let that assertion fail the day somebody
adds a terrain penalty. Then rename the number.

**Write the heuristic next to the step cost**, with a comment tying them
together, because a later change to one silently invalidates the other.

**Set `unknown` per caller, never globally.** The planner for a delivery run and
the planner for an exploration sweep want different answers from the same map.

**Test the failures.** No route, a goal in a wall, a goal off the map, a goal
under the robot. Each is a line of code and each is a real thing that happens.

## What Breaks First

- **A diagonal charged the same as a straight step.** See `E-NAV-0014`.
- **A heuristic that overestimates.** See `E-NAV-0015`.
- **A route through ground nothing has looked at.** See `E-NAV-0016`.

## Ship It

`plan_path` joins `rc::nav` beside the grid, the follower and the estimator. The
rover can now say where it is, decide where to go, and produce a route through
what it has actually seen, which is the whole of navigating a map.
