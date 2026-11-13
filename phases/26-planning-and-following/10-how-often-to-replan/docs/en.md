# How Often to Replan: Reasons, a Cooldown, and Planning From Where the Vehicle Will Be When the Plan Arrives

> A vehicle that never replans drives into the wall; one that replans every tick never drives at all.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 26-09, 26-04, 26-08

## The Problem

A plan is true when it is made and starts going out of date at once. The wall was not on the map; the person walked in front; the vehicle was pushed off the line. The question is **when to plan again**, and it has two bad answers. **Never**: the plan is followed into the wall. **Every tick**: the planner runs for ever, the path changes from one tick to the next, and the vehicle spends its time on what it is about to do and none on doing it.

## The Concept

### Reasons, not a timer

Plan again for a **reason**, checked each tick, in this order:

1. **The goal changed.**
2. **The path ahead is blocked** by something the latest grid knows: the cells along the next **3 m** of the path are looked at, not all of it (a wall 20 m away is not yet a problem).
3. **The vehicle is off the path** by more than 0.6 m.
4. **The plan is old**: 10 s, whatever else.

A **cooldown** (1 s between plans) holds back 3 and 4, so a vehicle that is wandering does not plan every tick. A blocked path is allowed to **bypass** the cooldown, because waiting costs a stop or a crash.

In a field with a wall found when the vehicle was 4 m from it, the vehicle that **never replanned** stood in front of it and **never arrived**. The one that planned for the reasons arrived in **20.9 s** with **5** plans. The one that planned every 0.2 s arrived in **21.4 s** with **86** plans: no quicker, and the planner never stopped. See `E-VEH-0109`.

### Look ahead far enough to hide the planning time

A plan takes time: the planner runs and the new path arrives, say, **2 s** later. The vehicle drives the old path meanwhile, if it can. If the blocked point is only noticed 0.6 m ahead, the vehicle has to stand still for the plan: it waited **2.1 s**. If it is noticed 3 m ahead, it keeps moving until the plan arrives: it waited **0.3 s**. The look-ahead has to be at least the speed times the planning time, and the safety stop (0.5 m here) is for when it is not. See `E-VEH-0110`.

### Plan from where the vehicle will be

A plan that takes 1 s and starts where the vehicle **was** arrives 1 s later, 1 m behind it: the new path starts **1.16 m** behind the vehicle, which has to turn round or cut back to it. That is more than the 0.6 m of the off-path reason, so the vehicle is **off its new path the moment it arrives**, and plans again, and again: **83 plans in 90 s, and it never arrived**. (The author did not expect this; the first run of the test failed to arrive and the count of plans showed why.) A plan started from where the vehicle **will be** (`predicted_pose`: the path, advanced by speed times the planning time) started **0.06 m** from it when it arrived, and the vehicle was at the goal in **20.1 s** with **5** plans. See `E-VEH-0111`.

## Build It

`should_replan` in `exercise/solution.hpp`.

```
rcpp verify 26-10
```

## Use It

`rc/drive/replan.hpp`.

```cpp
const rc::drive::ReplanReason why = rc::drive::should_replan(cfg, now, last_plan, pose, path, idx, grid, goal_changed);
if (why != rc::drive::kNoReplan) {
  const rc::drive::CarPose from = rc::drive::predicted_pose(pose, path, idx, speed, planning_time);
  start_planning(from);   // swap in the new path when it arrives, if it is still good
}
```

**What this does not do.** A new path is swapped in whatever it is like: there is no test that the new path is better than the old, so two equal paths on either side of an obstacle can be swapped between (a margin before switching, and a commitment to a path for a time, are the usual cures, and are left out). The prediction assumes the vehicle follows the path at a constant speed; a vehicle that is slowing or stuck is not where the prediction says. The sensor in the test is perfect within 4 m. The vehicle is a point that moves towards the next point; it has no dynamics (26-04, 26-07). A plan that fails (not found) leaves the old path active, which is wrong if the old path is blocked: the safety stop catches it, and 26-08 is the next step.

## What Breaks First

- **Never replanning, or replanning every tick.** See `E-VEH-0109`.
- **A look-ahead shorter than the planning time.** See `E-VEH-0110`.
- **A plan that starts where the vehicle was.** See `E-VEH-0111`.

## Ship It

`rc/drive/replan.hpp` holds `ReplanReason`, `ReplanConfig`, `first_blocked`, `should_replan` and `predicted_pose`. The module `rc::drive` now has ten headers: `hybrid_astar`, `dubins`, `smooth`, `follow`, `sampling`, `skid_follow`, `speed_profile`, `unstick`, `offroad` and `replan`.
