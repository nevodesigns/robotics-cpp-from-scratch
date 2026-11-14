# From Points to Obstacles: Clustering by Distance, With a Radius That Grows With Range Because the Points Do

> Points that are close belong to one thing, and how close is close depends on how far away they are.

**Type:** Build
**Time:** about 80 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 24-01, 24-02, 21-10

## The Problem

Lesson 24-02 left the points that stand above the ground. They are a cloud, and the planner needs objects: a post here, a person there, a wall. The step from points to objects is a grouping, and the grouping has a single parameter that decides everything: **how close two points must be to belong to the same thing**. Too small and every object falls to pieces, and a planner that drops small pieces sees nothing. Too large and neighbours are joined, and the gap between two posts that a vehicle could drive through is a wall.

## The Concept

### Connected by distance

`cluster_points` joins every pair of points closer than a distance, and takes the groups that are connected, directly or through other points. A group with fewer than `min_points` points is dropped, which is how scattered single returns are discarded. A group has its members, its centroid and the size of the box round it. The test scene had a post, a person, and two posts 0.8 m apart, with three scattered points. At a radius of 0.2 m it found **4 clusters**, with the centroids within **0.0000 m** of the truth, and the person was half a metre across.

### The radius

The same scene at other radii: at 0.02 m the points of a ring are further apart than that, so every point is alone and, with a floor of three points, **0 objects** were found; at 0.6 m the two posts 0.8 m apart (0.5 m between their faces) joined and there were **3**; at 5 m everything joined, and there was **1**. No radius is right for a scene whose objects are the same size at every distance, and for a lidar that is not so. See `E-VEH-0034`.

### The points spread with range

Lesson 24-01 showed that the points of a scan are `range * angle_inc` apart. A wall 11 m away, one degree between beams, has points **0.19 m apart**. A fixed radius of 0.15 m found **0** objects in it: every point alone, then dropped. A radius that grows with the range (here a base of 0.05 m plus one and a half times the beam spacing at that range) held the wall together as **1**. And the same scene has two posts at 2 m, 0.35 m apart, whose points are only 3.5 cm apart: a fixed radius of 0.4 m, big enough for the wall, joined them into **1** object, and the radius that grows with range kept them as **2**. `range_gain` is that growth; with it at 0 the radius is fixed. See `E-VEH-0035`.

## Build It

`cluster_points` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- For every pair of points closer than base_radius + range_gain * the larger distance from the origin, join their groups (union-find with the parent array).

As shipped, all 4 of the 4 tests fail. They are, in the order the suite runs them:

- Four objects among scattered points are found as four clusters, at the right places, and the scattered points are dropped.
- Too small a radius sees no objects, too large joins neighbours, and the right one lies between.
- A wall far away is broken into points by a fixed radius, and held together by a radius that grows with range.
- Two posts close together are one object to a radius that is too big for the near range, and two to one that grows with it.

```
rcpp verify 24-03
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/perceive/cluster.hpp` opens nothing new in `rc::perceive`.

```cpp
const auto objects = rc::perceive::cluster_points(above_ground, 0.05, 1.5 * angle_inc, 3);
for (const rc::perceive::Cluster& c : objects) track(c.cx, c.cy, c.size_x, c.size_y);
```

**What this does not do.** It compares every pair of points, which is quadratic and fine for the hundreds of points of one scan and not for a hundred thousand of a 3D cloud, where a grid or a tree finds the neighbours. It clusters in the plane and ignores height, so a branch over a post is joined to it. A person who is partly hidden is two clusters, and one thing that moves between scans is clustered again each time with no memory of the last, which is what tracking, in the last lesson of this phase, is for.

## What Breaks First

- **A radius chosen for one range.** See `E-VEH-0034`.
- **A fixed radius for a far wall.** See `E-VEH-0035`.
- **A gap between two posts joined by a radius that is too big.** See `E-VEH-0036`.

## Ship It

`rc/perceive/cluster.hpp` holds `Cluster`, `cluster_root` and `cluster_points`.
