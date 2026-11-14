# Finding the Ground: A Plane Fitted by Random Samples, Why a Least Squares Plane Is Pulled Off by What Stands on It, and a Ground That Is Not the Biggest Plane

> The ground is the plane that most of the points agree on, and the points that stand on it are the vote it must ignore.

**Type:** Build
**Time:** about 85 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 24-01, 22-02, 21-10

## The Problem

The points of lesson 24-01 are all alike. Some of them are the ground, which the vehicle drives on and must not mistake for an obstacle; some are things standing on it, which it must not drive into. A vehicle on a slope sees the ground tilt, and a ground that is taken to be level is a floor that rises into a wall. So the first job on a point cloud is to find the ground, and the easy way to do it is the one that goes wrong: fit a plane to everything and take it to be the ground.

## The Concept

### The least squares plane is pulled off

`fit_plane_least_squares` finds the plane `z = a x + b y + c` that makes the squared heights of all the points least. On a scene of 2000 ground points tilted 5 degrees, with a centimetre of noise, it found the tilt to 0.1 degree. With 450 more points on three boxes up to 1 m high, towards the far end, it found **6.57 degrees**: the boxes' points are above the ground, a plane that minimises squared distances goes up to meet them, and it tilts. The more there is on the ground, the worse it gets, and a vehicle among obstacles is where it is least wanted. See `E-VEH-0031`.

### Random samples

`fit_plane_ransac` does not fit anything. It draws **three points at random**, makes the plane through them, counts how many of all the points are within a tolerance of it (3 cm here), and keeps the plane with the most. A draw that includes a point on a box gives a bad plane, with few points near it, and loses; a draw of three ground points gives a good one, with nearly all the ground, and wins. The obstacles are not corrected for, they are simply not counted. It found **5.08 degrees** with **82 percent** of the points within 3 cm. The same seed draws the same points, so the answer is repeatable.

How many draws? One is clean when all three points are on the ground, which has a probability of the share cubed; `ransac_samples` gives the number that make at least one clean with a confidence: at this share, **5 draws** for 99 percent, and a hundred times as many if only 30 percent of the points are on the plane. Two hundred is far more than needed here. See `E-VEH-0032`.

### Ground and what stands on it

`split_ground` separates the points within a height of the plane from those above it. With 10 cm, the boxes came out as **402 points above** the ground, and each of them is more than 10 cm above the plane.

### The biggest plane is not always the ground

A wall can hold more points than the ground does, and then the biggest plane is the wall. A scene of 600 ground points and 1200 on a wall gave a plane tilted **90 degrees** to an unconstrained search: a vertical wall. A search that refuses any plane tilted by more than 30 degrees gave **0.2 degrees**, the ground, with fewer points. The constraint is a piece of knowledge about the vehicle, which drives on ground that does not go up by more than a limit, and it costs nothing to add. See `E-VEH-0033`.

## Build It

`fit_plane_ransac` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- For each draw pick three different points, make a plane, skip it if it is too tilted, count the points within the tolerance, and keep the plane with the most.

As shipped, 3 of the 5 tests fail, and these are the ones your code has to turn green:

- What stands on the ground pulls a least squares plane off it and does not move a plane of random samples.
- The ground and what stands on it are split by height above the plane.
- A wall with more points than the ground is the biggest plane, and is the ground only if nothing says the ground is level.

The other 2 pass already, because they test the parts that are given.

```
rcpp verify 24-02
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/perceive/ground.hpp` opens nothing new in `rc::perceive`.

```cpp
const rc::perceive::RansacResult r = rc::perceive::fit_plane_ransac(points, 100, 0.03, 30.0, 1);
const rc::perceive::GroundSplit s = rc::perceive::split_ground(points, r.plane, 0.10);
```

**What this does not do.** It finds one plane, and a ground that is a slope, then a flat, then another slope is not one plane: for that the cloud is cut into cells and a plane fitted to each (the next lessons do something like it as a grid). The tolerance and the height are parameters chosen by hand, and a ground with a noise of more than a few centimetres needs them larger. It uses no knowledge of the order of the points in the scan, which a real ground filter uses, and it does not refine the plane after the vote.

## What Breaks First

- **A plane fitted to everything.** See `E-VEH-0031`.
- **Too few draws for the share of ground.** See `E-VEH-0032`.
- **A wall taken for the ground.** See `E-VEH-0033`.

## Ship It

`rc/perceive/ground.hpp` holds `Plane`, `plane_distance`, `plane_from_points`, `tilt_degrees`, `fit_plane_least_squares`, `RansacResult`, `fit_plane_ransac`, `ransac_samples`, `GroundSplit` and `split_ground`.
