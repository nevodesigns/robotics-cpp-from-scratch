# Growing a Tree Through the Clutter: RRT and RRT-Connect for an Arm, Why Two Trees Beat One, and the Planner That Must Know When to Give Up

> A sampling planner does not know the shape of the space; it finds out by trying, and a path is what is left when it has tried enough.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 30-02, 16-06

## The Problem

Phase 16 planned a path across a grid for a rover. An arm cannot do that. Its **configuration space** has six dimensions (one for each joint), too many to lay a grid over: ten steps a side is a million cells and a hundred is a million million. And **the obstacles are not in that space**: a table is a box in the workspace, but in joint space it is a twisted region with no simple description, and the only way to know whether a point of it is free is to **ask** the collision model (30-01). A planner for an arm has to find a path using only that question.

## The Concept

### Grow a tree by trying

**RRT** (the rapidly exploring random tree) starts with the start configuration as a tree of one node and repeats:

1. **Sample** a configuration at random inside the joint limits (the **goal itself** one time in ten, which pulls the tree towards it).
2. Find the **nearest node** of the tree.
3. **Steer** towards the sample by at most a **step length** (0.5 rad here).
4. If the edge to the new point is **free** (conservative advancement of 30-02: no thin thing is stepped over), **keep** it, with the nearest node as its parent.

When a node can be joined to the goal by a free edge, the path is read back through the parents. The tree needs to know nothing about the shape of the free space: it **finds out by trying**, and the nearest-node rule makes it grow into the large unexplored regions first (hence "rapidly exploring"). See `E-ARM-0058`.

### Two trees

**RRT-Connect** grows a tree from the **start** and one from the **goal**. Each iteration extends one tree towards a sample; then the **other** tree is extended **repeatedly** towards that new node until it joins it or is blocked; then the trees **swap**. A path exists the moment they meet. A tree reaching out from each end meets the other far sooner than one tree that has to find a goal that is a small target in a large space. Across **6** seeds, in a scene with a wall between the start and the goal, RRT-Connect found a path in **6** and RRT in **3**, with **23** nodes on average against **58**. Every edge of every path was free by the conservative check. See `E-ARM-0059`.

### The path is ugly

A path from a random tree wanders: it was **2.90 times** as long as the straight line between the ends, with corners where the tree turned. It is a path that is **free**, not a path that is **good**. The next lesson shortens and smooths it.

### Knowing when to stop

A planner that finds nothing cannot know whether there is nothing to find. With the **goal inside the wall** the planner says so **at once** (the goal fails the pose check: **0 iterations**), and with a goal that is free but not reachable in the iterations given, it stops at its **limit** (here 5 iterations with a tiny step: it stopped after exactly **5**). A planner that runs for ever on an impossible request is a robot standing still. See `E-ARM-0060`.

## Build It

`steer` in `exercise/solution.hpp`.

```
rcpp verify 30-03
```

## Use It

`rc/arm/rrt.hpp`.

```cpp
rc::arm::PlanProblem p;  p.check = check;  p.bounds = bounds;  p.seed = 7;
const rc::arm::PlanResult r = rc::arm::rrt_connect(p, q_start, q_goal);
if (r.found) smooth_and_time(r.path);
```

**What this does not do.** The paths are **not optimal** and are **not repeatable** across seeds (the same seed gives the same path, a different one another). There is no guarantee of **finding** a path in a given time, only that, if one exists, the chance of not finding it falls as the iterations go up (probabilistic completeness); a **narrow passage** (a gap that the arm only just fits through) takes far longer, and 30-04 shows why. The nearest-node search is a **linear scan**; for tens of thousands of nodes a tree structure is needed. The distance is Euclidean in **joint** angles, which treats a radian of the shoulder as the same as a radian of the wrist (it is not: the shoulder sweeps the whole arm). The joint limits are sampled, **not the wrap of angle**. There is no **time** in it: a path of poses, not a motion (30-06).

## What Breaks First

- **A planner that needs the shape of the free space.** See `E-ARM-0058`.
- **One tree for a small goal.** See `E-ARM-0059`.
- **A planner with no limit.** See `E-ARM-0060`.

## Ship It

`rc/arm/rrt.hpp` holds `PlanRng`, `PlanProblem`, `PlanResult`, `joint_distance`, `steer`, `Tree`, `sample_config`, `ExtendStatus`, `extend`, `rrt`, `rrt_connect` and `path_length`.
