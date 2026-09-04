# The Rover That Fits: Width, Clearance and Replanning

> The route was planned with the robot's radius inflated into the map. The robot
> passed 6.8 cm from the rack. Every number in the report was correct.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 16-06, 16-04

## The Problem

There is a map, a search over it, and a follower that drives what the search
returns. Three lessons, three pieces, and a rover that navigates a map.

Except that a search over cells plans for a point, because a cell is a point,
and the robot is a disc. And the route it returns is a photograph of the map at
the moment it was taken, which stops being true as soon as anything moves.

This lesson closes the phase by putting the robot's own width into the map, and
then by measuring how much of that margin the follower spends before the robot
gets any of it.

## The Concept

### The shortest route goes through the gap you do not fit through

A room with a wall across it, a 0.40 m gap straight ahead and a 1.50 m gap
twenty cells off to one side. Start and goal 6.9 m apart in a straight line:

| robot radius | route | length | clearance |
|---|---|---|---|
| 0.00 m | found | 6.900 m | **0.200 m** |
| 0.15 m | found | 7.977 m | 0.224 m |
| 0.25 m | found | 8.060 m | 0.316 m |
| 0.35 m | found | 8.143 m | 0.412 m |
| 0.50 m | **none** | | |

Treated as a point, the robot takes the near gap and passes 20 cm from the wall,
which a robot of 25 cm radius does by hitting it.

That is not bad luck. A narrow gap is usually the direct way, which is why it is
there, so **the shortest route is shortest because it goes through the gap the
robot does not fit through**.

The bottom row is the other half of the answer. A robot too wide for either gap
gets no route at all, rather than a route it cannot drive. That is the outcome
you want and the one a point planner cannot give you.

### Grow the obstacles, not the search

```cpp
const auto planning = rc::nav::inflated(map, robot_radius);
const auto plan = rc::nav::plan_path(planning, start, goal, options);
```

Checking the robot's footprint at every step of the search would work and would
cost that check tens of thousands of times per plan. Growing the obstacles does
the geometry once, into the map, and leaves the search a search over points.

Four details, each a decision rather than an implementation note.

**A disc, not a square.** Skip a neighbour when `dx*dx + dy*dy` exceeds the
radius squared. A square inflation over-blocks the diagonals by root two and
closes gaps the robot would fit through.

**Round the radius up to whole cells.** Half a cell does not exist and rounding
down makes a map that says the robot fits when it does not. Inflation is then
conservative by up to one cell, which belongs in the margin rather than being a
surprise.

**Off the edge of the map is occupied.** A robot half over the border is
somewhere the map cannot describe.

**Unknown grows as unknown.** A cell beside unseen ground is not known to be
blocked, and calling it blocked makes the map more certain than the evidence.
What the planner does with unknown stays the planner's decision, from 16-06.

### The margin is spent three times

Here is the measurement the lesson exists for. Take that 0.25 m route, whose own
tightest point clears 0.316 m, and drive it with the follower from 16-04
steering by a pose 100 ms old:

| | robot's own worst clearance |
|---|---|
| the path itself | 0.316 m |
| lookahead 0.30 m | 0.337 m |
| lookahead 0.60 m | 0.255 m |
| lookahead 1.00 m | **0.068 m** |

At a metre of lookahead the robot passes 6.8 cm from an obstacle, on a route
planned with a 25 cm radius inflated into the map, having arrived safely and
reported a small cross track error the whole way.

Nothing failed. The follower cut the corners of the route, which is exactly what
a lookahead does and what `E-NAV-0009` measured at about 0.27 of it for a right
angle, and that cut came out of the margin the inflation was supposed to
provide.

**Three things claim the same margin**, and inflation covers one:

```
margin = robot radius
       + 0.27 * lookahead                  (the follower's corner cut)
       + speed * estimator latency         (where the robot thinks it is)
       + half a cell                       (the map's own quantisation)
```

Every term is measurable and three of them were measured in earlier lessons.

It runs the other way too, and that is the useful direction. Shortening the
lookahead buys clearance directly. So does reducing the estimator's latency. A
robot that cannot fit down an aisle at full speed may fit down it slowly, and
now that is a calculation rather than an experiment.

### Measure the robot, not the path

The number a clearance requirement is about is how close the **robot** came, and
it is neither the planner's clearance nor the follower's tracking error.

```cpp
const double here = rc::nav::clearance_at(map, pose.x, pose.y);
```

Log the worst over a drive. A route that tracks perfectly and passes 5 cm from a
rack is a route that hits the rack, and the tracking figure will say 0.03.

### A plan is a photograph

Plan a route across the room, then set a pallet down where it crosses the gap.
Five cells of the plan are now inside an obstacle, and the plan still says to
drive through them. The map is correct; the sensors saw the pallet; the display
shows it. Nothing asked the question again.

```cpp
for (const auto& cell : remaining_cells)
  if (inflated_map.classify(cell.x, cell.y) == rc::nav::Cell::occupied) { replan(); break; }
```

Checking is a walk down a few hundred cells. Replanning expands tens of
thousands. That difference is what makes checking affordable at the control rate
and replanning affordable only on demand.

Three things go with it. Check only the part not yet driven, because an obstacle
behind the robot is history. Check the inflated map, or the check passes until
the robot's edge touches something. And have an answer for a replan that fails,
because stopping is a valid outcome and a robot that keeps driving the old route
is worse than one that stops.

Replanning on a timer as well is worth it where the map improves rather than
merely degrades: a route through unknown ground gets better as the ground is
seen, and nothing about that trips an obstacle check.

## Build It

Implement `inflated`, `clearance_at` and `clearance_along` in
`exercise/solution.hpp`.

```
rcpp verify 16-07
```

The suite inflates a single obstacle and checks the shape of what grows, plans
the room five ways, drives the result at three lookaheads, and puts a pallet
down on a route that has already been planned.

## Use It

**Inflate for the sum, not for the radius.** Write the four terms out where the
number is set, so the next person changing the lookahead can see what they are
spending.

**Report the robot's worst clearance from every acceptance drive.** It is one
line to log and it is the only number that answers the question anybody actually
asked.

**Check the remaining route against the inflated map every tick.** Replan when
it fails, and be able to stop when the replan does.

## What Breaks First

- **A route planned for a robot with no width.** See `E-NAV-0017`.
- **Clearance measured on the path rather than on the robot.** See `E-NAV-0018`.
- **A plan that outlived its map.** See `E-NAV-0019`.

## Ship It

`inflated`, `clearance_at` and `clearance_along` join `rc::nav`, and phase 16 is
finished. A rover that knows where it is, knows what it has seen and has not,
plans a route through what it knows, drives that route, fits down it, and
notices when the room changes. Every one of those is a number rather than a
hope, and the numbers add up in one place.
