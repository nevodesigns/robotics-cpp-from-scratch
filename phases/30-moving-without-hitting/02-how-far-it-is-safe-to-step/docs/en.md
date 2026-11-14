# How Far It Is Safe to Step: Checking a Motion and Not Just a Pose, a Bound on How Fast the Arm Can Sweep, and a Thin Obstacle That Fixed Steps Miss

> A pose is free or it is not; a motion is free only if nothing between the poses was missed.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 30-01, 28-03

## The Problem

30-01 can say whether one pose of the arm collides. A planner does not ask for a pose; it asks for a motion: can the arm go from here to there? A motion is a continuous path through infinitely many poses, and a program can look at only a few. Look at too few and a thin obstacle between two of them is missed, and the arm drives through it. Look at too many and the planner is slow. The question of how far apart the poses can safely be has an answer, and it is not "small".

## The Concept

### How far can a point of the arm move?

For a revolute joint turning through an angle, a point of the arm beyond it moves along a circle, and so no further than the angle times its distance from the axis. That distance is at most the sum of the lengths of the links from that joint outward (they are constants of the arm, and do not depend on the angles), plus the tool and the radius of the fattest capsule. That is the sweep bound of the joint: for the typical arm the largest is 1.33 m for a radian. Add the joints up: for a change `dq` of the joint angles, no point of the arm moves more than the sum of each joint's bound times its change. The bound is not tight (it assumes everything beyond the joint is stretched straight out); it holds in every pose, which is what matters. Over 200 random moves no point of the capsules moved further than it said: the largest ratio was 0.289.

### Fixed steps miss thin things

The simple way to check a motion is to check the poses along it at equal steps. A sphere of radius 5 mm placed on the path of the tool of a move of 2.0 rad (at a share of the path that the steps do not land on) was missed by steps of 0.5 rad (found: 0; 1 is found, 0 is missed) and found by steps of 0.01 rad, which looked at 70 poses. A thin obstacle is not unusual: a wire, an edge, a person's finger. See `E-ARM-0055`.

### Conservative advancement

The bound makes a guarantee possible. At a pose the clearance (the distance to the nearest obstacle, 30-01) says how far anything of the arm can move before touching it. Divide it (less a margin) by the sweep of the whole edge, and that is the share of the edge that can be covered without any point of the arm reaching the obstacle: step that far. Far from everything the steps are large; near an obstacle they shrink as the clearance shrinks, and nothing can be stepped over. The thin sphere was found (1), in 14 poses. On an edge nowhere near anything it looked at 2 poses where fixed steps of 0.01 rad looked at 201: fewer when it can be, more where it has to be. See `E-ARM-0056`.

### A margin

A clearance of 3 cm is free with no margin and not free with a margin of 5 cm. The margin is how much room the arm should keep: for a controller that tracks a path with an error (29-05), for a person near it, for the radii that are a little too small (30-01). It belongs in the check, not in the obstacle. See `E-ARM-0057`.

## Build It

The step of `edge_conservative` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- No point can move more than the sweep of the whole edge times the share of it covered: so the share that can be covered is (clearance - margin) / full sweep.

As shipped, 2 of the 4 tests fail, and these are the ones your code has to turn green:

- A thin obstacle between the steps is missed by fixed steps and found by conservative advancement.
- Far from anything conservative advancement looks at far fewer poses than fixed steps.

The other 2 pass already, because they test the parts that are given.

```
rcpp verify 30-02
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/arm/motion_check.hpp`.

```cpp
const auto bounds = rc::arm::sweep_bounds(table, radii, tool_length);
rc::arm::ArmCheck check = {&table, &radii, tool_length, &scene, true};
const rc::arm::EdgeResult e = rc::arm::edge_conservative(check, q_from, q_to, bounds, 0.02);
```

**What this does not do.** The bound is for revolute joints and a straight line in joint space; a curved path is checked piece by piece. It is pessimistic: it assumes everything beyond a joint is at full stretch, and so a conservative check looks at more poses than it must when the arm is folded. The clearance is of the model (30-01), not of the arm. The minimum step (1e-4 of the edge) is there so that a clearance of exactly zero cannot freeze the loop; it means that a thin obstacle that is inside a pose already is found only by the pose check. Nothing here is fast: the cost of a clearance query was not measured, and a planner that makes millions of them wants the bound and the queries tuned. Moving obstacles are 30-07 and 30-08.

## What Breaks First

- **A motion checked at a few poses.** See `E-ARM-0055`.
- **A check that is thorough everywhere.** See `E-ARM-0056`.
- **A margin put in the wrong place.** See `E-ARM-0057`.

## Ship It

`rc/arm/motion_check.hpp` holds `sweep_bounds`, `sweep_distance`, `ArmCheck`, `clearance_at`, `free_pose`, `EdgeResult`, `edge_fixed` and `edge_conservative`.
