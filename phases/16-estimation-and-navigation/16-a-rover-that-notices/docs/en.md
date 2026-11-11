# A Rover That Notices: Plan, Drive, Sense, Replan, and What Each Choice Costs

> Every piece of this phase can be right and the rover can still drive into the door. The pieces have to be arranged.

**Type:** Capstone
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 16-06, 16-15, 16-12

## The Problem

Phase 16 gave the rover odometry, fusion, a map, a planner, a way to know it is where it thinks, a sense of when the map is stale and a place to go next. None of it drives. A rover is a loop: look, decide, move, look again. How the loop is arranged decides what the pieces are worth, and each choice in it
has a price that can be measured.

This lesson builds the loop for a rover that starts knowing nothing, with three choices left open, and compares them on the same building with the same door.

## The Concept

### The loop

```
every step:
  apply any change to the world
  sense the cells within a radius, into the map
  if at the goal: done
  plan, if the policy says so
  move one cell along the plan, or bump if the world has a wall there
```

The building is 40 by 30 cells with a wall down the middle, a door three cells wide at the centre and a gap at the far end. The rover starts at one side and the goal is at the other, 33 moves apart in a straight line through the door. The map starts as all unknown. The rover senses cells within 6 of it, and moves one cell at a time, up, down, left or right.

### In a building that does not change

| | outcome | steps | plans | bumps |
|---|---|---|---|---|
| replans when blocked | reached | 33 | 1 | 0 |

33 is the fewest moves the building allows, checked against the library planner run on the true building. One plan was enough. The planner went through unknown cells at a higher cost, found the straight way through the door, and the map filled in under the rover as it went. See `E-NAV-0046`.

### The price of planning

| | steps | plans |
|---|---|---|
| replan when blocked | 33 | 1 |
| replan every step | 33 | 33 |

Planning every step costs a plan for every move and does not make a better route here, because the first was already the best. Where the world is changing it might, which is the next table. See `E-NAV-0044`.

### Unknown is not a wall

With every unseen cell treated as a wall, the rover standing in a map that knows nothing has nowhere to plan to: no route, after 0 steps. With unknown cells allowed at a cost it reached the goal in 33. Treating unseen space as impassable is safe and means the rover never leaves. See `E-NAV-0046`.

### The door that closes

The door closes at step 15, when the rover is near it.

| policy | outcome | steps | plans | bumps |
|---|---|---|---|---|
| never replans | not reached | 300 | 1 | 284 |
| replans when blocked | reached | 83 | 6 | 0 |

A rover that planned once is still pushing at the door at step 300, having bumped 284 times. One that notices its next step is a wall, replans round and arrives at 83 steps with 6 plans. The fewest moves from the start with the door shut is 61, and the rover had already driven 15 toward the door, so 83 is a detour of its own making, not one
of poor planning. See `E-NAV-0044`.

### Believing the sensor

The map is an average, built to ride out noise. A cell that has been free for a long time takes several observations of a wall to turn into one. Compare a rover that believes a contradiction at once, a cell the map says is free and the sensor says is a wall, with one that lets the map average it:

| policy | outcome | steps | plans | bumps |
|---|---|---|---|---|
| believes a contradiction at once | reached | 83 | 6 | 0 |
| lets the map average it away | reached | 87 | 6 | 4 |

Four bumps and four extra steps, from a door. The cost of averaging is the number of scans the map needs to forget, which lesson 16-15 measured. Believing at once costs nothing in a world that is not noisy, and in a noisy one it needs the persistence that lesson's detector has. See `E-NAV-0045`.

### When there is no way

Every way through, the door and the gap, shut at step 0. The rover stopped with no route after 54 steps and 7 plans: it went to look, replanned as it found each wall, and the planner finally said there was nothing. A loop that ignored the planner's answer would have kept bumping until its step limit. The stop is a result, with a count of how it
got there, and it is what a supervisor needs to hear. See `E-NAV-0046`.

## Build It

`sense_around` and `drive_to_goal` in `exercise/solution.hpp`. The policy, the change and the result types and `force_cell` are given.

```
rcpp verify 16-16
```

The building is built cell by cell in the tests, and nothing is random, so the figures above are the same on every run.

## Use It

`rc/nav/rover.hpp` joins `rc::nav`.

```cpp
RoverPolicy policy;
const RoverResult r = drive_to_goal(world, start, goal, policy, 6, WorldChange(), 300);
if (!r.reached) report(r.no_route, r.bumps, r.plans);
```

**What this does not do.** It is a grid world with exact motion: no odometry error (lesson 16-01), no pose correction (lesson 16-13), no speed profile (lesson 16-11) and no path shortening (lesson 16-10). It senses a disc of cells with no line of sight and no noise, which flatters it. It replans from scratch and does not
explore on purpose (lesson 16-12 chooses where). It is a way to measure the choices, and a real rover swaps each piece in turn and measures again.

## What Breaks First

- **Planning once.** The first plan meets the first closed door. See `E-NAV-0044`.
- **A map that forgets slowly.** Bumps and wasted steps while it catches up. See `E-NAV-0045`.
- **Unknown as a wall, or ignoring a no route.** A rover that never leaves, or one that never stops. See `E-NAV-0046`.

## Ship It

`rc/nav/rover.hpp` holds `RoverPolicy`, `WorldChange`, `RoverResult`, `force_cell`, `sense_around` and `drive_to_goal`.

This ends phase 16: a rover that plans a route through what it has actually seen, drives it, and notices when the room changes. Fitting down it (lesson 16-07) and pulling it tight (lesson 16-10) are the pieces it is built from.
