# Getting Out of the Way of Something That Moves: Guessing Where It Is Going, Looking Along the Plan for the Moment They Meet, and Choosing Between Waiting and Going Round

> A plan for a world that moves is checked every tick, and the cheapest safe answer is sometimes to stand still.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 30-07, 30-06, 30-05, 26-10

## The Problem

30-07 gave the arm a scene that changes. A change can be **slow** (a box moved between picks) and then a recheck when the version changes is enough. It can be **fast**: a person's hand, a part on a conveyor. The thing is **moving into the arm's path while the arm is moving along it**, and a plan that was free when it was made is not free a second later. The arm has to **look along its own future** every control tick, and know what to do when it sees the thing coming.

## The Concept

### Guess where it is going

Seen once, a thing is a point. Seen twice, it has a **velocity**: the difference of the two places over the time between. A **track** keeps the last place and a velocity smoothed over sightings (a share of 0.7 of the new difference and 0.3 of the old), and **predicts** a place a time later by the straight line. A thing seen every 0.1 s moving at 0.5 m/s gave a speed of **0.499 m/s** after six sightings (a true **0.5**). The guess is a **straight line at constant speed**: it is wrong for a hand that stops or turns, so the monitor runs again at every tick.

### Look along the plan for the meeting

The arm has a **trajectory** (30-06), so it knows where it will be at every moment of the next second or two. The track says where the obstacle will be. **Step along both together** and ask whether the arm's capsules (30-01) and the obstacle are within a margin. The first yes is the **time to collision**; none within the horizon, a negative number. An obstacle 1 m from the tool coming at 0.5 m/s was **1.78 s** away, and one going away was never found. See `E-ARM-0073`.

### An arm that does not look hits it; one that does stops

A sphere of 8 cm rising through the arm's path at the moment the tool reached the middle of it was **hit** by an arm that did not look (**-0.112 m**, negative: through it). An arm that **looked** and **stopped** when a meeting was predicted within a second was **0.031 m** from it at the closest, and took **2.15 s** against **1.50 s** for the same move with nothing in the way. See `E-ARM-0074`.

### Waiting or going round

An arm that stops can also **plan again from where it stands** (RRT-Connect and shortcutting, 30-03 and 30-05, against the obstacle made a little larger) and go round. It was **0.135 m** from the obstacle, safe, and took **4.20 s** with **1 plan**: **slower than waiting**. Going round a thing that is about to **go away** is a long detour for nothing. Which is right depends on whether the thing **leaves** or **stays**, and a monitor cannot tell: a sensible arm waits for a moment and then plans. See `E-ARM-0075`.

### Still looking after the plan

A plan made against where the obstacle is now can be run into by the obstacle later. The monitor looks along the **new** plan too. With nothing extra planned for the obstacle to move into, the arm still got round with no contact.

### When the arm is inside the room that planning needs

If the extra room kept from the obstacle is so large that **the arm's own pose is inside it**, the planner finds no plan and the arm **waits**. It did, and it was safe, because the thing was going away (waited: **1**, 1 for yes).

## Build It

`time_to_collision` in `exercise/solution.hpp`.

```
rcpp verify 30-08
```

## Use It

`rc/arm/moving_obstacle.hpp`.

```cpp
rc::arm::Track track;  rc::arm::track_update(track, sighting, now, 0.7);
const double ttc = rc::arm::time_to_collision(table, radii, tool, trajectory, traj_time, track, now, obstacle_radius, margin, 1.0, 0.05);
if (ttc >= 0.0) stop_and_wait_or_replan();
```

**What this does not do.** The **stop is instant** in this simulation: a real arm needs a stopping distance, and the horizon has to be at least the time to stop. The **prediction is a straight line** and the obstacle is a **sphere** with a known radius. The sightings are **perfect and prompt**. The arm **cannot retreat**: a big thing that comes to a stopped arm is not avoided by standing still. There is a **single obstacle**. The decision between waiting and planning is a **flag**, not a rule.

## What Breaks First

- **A plan that is only checked when it is made.** See `E-ARM-0073`.
- **An arm that does not stop for what is coming.** See `E-ARM-0074`.
- **Going round what would have gone away.** See `E-ARM-0075`.

## Ship It

`rc/arm/moving_obstacle.hpp` holds `Track`, `track_update`, `track_predict`, `time_to_collision`, `ReplanConfig`, `ReplanRun` and `run_with_moving_obstacle`. With it phase 30 is complete: an arm that knows its own shape and its surroundings, plans a motion that touches nothing, makes it short and smooth, times it within the limits of its joints, and plans again when the world changes.
