id: E-FIELD-0029
title: A pass judged on the last sample
match: the run reached the goal but went through a wall to get there
match: success is the distance to the goal at the end
platforms: linux, windows
teaches: 22-10-scoring-a-run
---

## Symptom

Every run passes and the robot is seen to scrape a wall.

## Cause

Only the final position was judged. The score now carries the clearance (down to -0.55 m inside a post) and the collisions.

## Fix

Score the whole run: goals in order, clearance, collisions, time, straightness, and give thresholds to each.

```cpp
const NavScore s = score_navigation(scenario, log, robot_radius);
```
