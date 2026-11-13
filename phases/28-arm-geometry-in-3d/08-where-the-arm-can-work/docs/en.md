# Where the Arm Can Work: Reach by Sampling, the Shorter Reach With the Tool Down, the Smaller Region Where It Can Move Well, and What Fits on a Table

> Reach is a number on a data sheet; the place where the arm can actually do something is a smaller shape.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 28-07, 28-04, 13-10

## The Problem

A data sheet says an arm has a **reach** of, say, 850 mm. That is the radius of the sphere the tool can touch the outside of with the arm stretched. It says nothing about what the arm can **do** inside it. A pick and place cell is not designed around the reach; it is designed around **where the arm can put its tool pointing down, away from singularities and inside its limits.** That region is smaller and has to be found.

## The Concept

### Reach by sampling

The most direct way to find where the tool can go is to **ask**: draw joint angles at random inside the limits, put them through forward kinematics (28-02), and mark the cell of a grid (a **voxel**) that the tool is in. No inverse kinematics, no assumptions about the shape. The grid then holds the reachable volume.

The estimate is only as good as the **sample**, and the sample has to be **many to each cell**. In cells of 0.10 m the typical elbow arm's volume came out at **1.081 m3** from 2 thousand random poses, **2.581** from 20 thousand and **2.958** from 200 thousand: the small sample **badly underestimates**, because a point near the edge of the reach is reached by very few combinations of angles, and the count levels off only when the sample is much bigger than the number of cells. In cells of half the size, eight times as many, 60 thousand poses gave **2.033 m3** and 200 thousand **2.554 m3**: **still rising**, so not an answer yet. A count of the cells marked that is still growing with the sample is not a volume. See `E-ARM-0022`.

The farthest reach found for the UR5 was **1.05 m** from its base and for the Panda **1.16 m**; both are no more than the sums of the lengths and offsets of their tables allow, and both are most of that sum.

### Not a ball once the orientation is asked for

For **position alone** the reachable region of this arm is nearly a solid ball: the flange can reach into the space round the shoulder (the last link is longer than the difference of the upper arm and the forearm), so there is **no hole** to speak of. (The author expected one and looked for it; there is not one for this arm. The UR5 and the Panda have links of near lengths too, and what keeps them from their own bodies is **self-collision and joint limits**, which are not here.) The shortfall comes when the **orientation** is asked for: at table height the farthest the tool could reach in any orientation was **0.86 m** from the base axis, and **pointing straight down only 0.74 m**. That is the difference between the data sheet and the task. See `E-ARM-0023`.

And not everywhere can the arm **move well**. Marking only those poses whose smallest singular value (28-06) is at least 0.1 (the **dexterous** ones) gave **62 percent** of the reachable volume. The rest is reachable, and an arm there is near a singularity: it can be put there, and cannot be taken out in the direction you want.

### What fits on a table

A cell is designed around **tasks**: "the tool pointing down, at every point of a rectangle on a table 0.15 m high". For the arm with a spherical wrist this is a question for the closed form of 28-04, per point, **inside the joint limits**. The elbow arm could reach **96 percent** of a table 0.15 m high from 0.15 to 0.70 m in front of it and 0.4 m either side, with no joint limits; **96 percent** with limits of plus and minus 120 degrees (wide limits cost nothing here: the lesson found that out by measuring, having expected a loss); and **78 percent** with limits of plus and minus 60 degrees. See `E-ARM-0024`.

## Build It

`tool_down_reachable` in `exercise/solution.hpp`.

```
rcpp verify 28-08
```

## Use It

`rc/arm/reach3d.hpp`.

```cpp
rc::arm::VoxelGrid g(0.05, 1.2);
rc::arm::sample_reach(table, base, tool, 200000, 1, 0.1, g);   // the dexterous poses
const double v = rc::arm::volume(g);
const double covered = rc::arm::table_coverage(table, tool, x0, x1, y0, y1, z, 0.05);
```

**What this does not do.** The sampling gives **positions** only, not the orientations that can be had at each: the same cell may be reachable with the tool pointing up and not down, which only the tool-down question (for one arm) answers. The tool-down question is for **arms with a spherical wrist** (28-04); for the UR5 or the Panda it would use the iteration of 28-05 from several starts. There is no **obstacle** in it: a table, a wall or the arm's own links; collisions are phase 30. The singular value mixes metres and radians (28-06), so the 0.1 is a choice. A voxel is a coarse way to hold a volume and the numbers depend on its size.

## What Breaks First

- **A reach found with too few samples.** See `E-ARM-0022`.
- **A reach taken for a working area.** See `E-ARM-0023`.
- **A table that the data sheet says is in reach.** See `E-ARM-0024`.

## Ship It

`rc/arm/reach3d.hpp` holds `VoxelGrid`, `sample_reach`, `volume`, `farthest`, `farthest_horizontal`, `tool_down_reachable`, `tool_down_radius` and `table_coverage`.
