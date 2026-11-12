# Matching One Scan to Another: Iterating Nearest Points to a Rigid Move, Where It Converges, and the Corridor It Cannot Tell Anything About

> A scan match gives the move that makes two pictures agree, and it says nothing about directions in which they always agree.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 25-02, 24-03, 22-02, 21-10

## The Problem

A robot that has two scans, one a moment before the other, has two pictures of the same place, and the move of the robot between them is the move that puts one picture on the other. Finding that move is **scan matching**, and it is the most direct way for a robot to know how far it has gone without trusting a wheel: it looks. The commonest method is simple to state and delicate to use: **iterative closest point**.

## The Concept

### The loop

Given a first guess of the move, move the source scan by it. Pair each source point with the **nearest** target point, if that is within a distance. For those pairs there is a rigid move, a rotation and a translation, that puts them together with the least sum of squared distances, and it has a closed form in two dimensions: centre both sets on their means, the rotation is `atan2(Σ cross, Σ dot)` of the centred pairs, and the translation takes the rotated source mean onto the target mean. Make that move, add it to the guess, and repeat, until a round moves almost nothing. `icp` is that loop.

A scan of an L shaped room with a post in it, moved by 0.30 m, 0.10 m and 0.15 rad with 1 cm of noise on every point, was matched with **no first guess** to **0.0007 m** and **0.0001 rad** in **16 rounds**.

### It converges from near, and not from far

Each round pairs a point with the nearest, not the right one, and while the guess is far off, the nearest is often wrong. The method finds the nearest minimum, which is the right one only if the guess is near enough. The match converged from first guesses wrong by up to **0.5 rad** in rotation, and **not from 0.8 rad**: the L shape has other places where it fits itself. How big this *basin* is depends on the shape. The cure is a good first guess, and that is the odometry: from a guess close to the truth the match took **5 rounds**, against **16** from none. See `E-VEH-0064`.

### What a scan cannot tell

In a long straight corridor every position along it looks the same. A scan of two parallel walls has no information about how far along them the sensor has moved, and the match is free to slide. With a real move of 0.5 m along the corridor and an odometry guess of 0.4 m, the match found the cross-corridor offset and the heading to a few millimetres and left the move along it exactly as the guess, **0.10 m** wrong. A partition across half the corridor, scanned closely enough that it has as many points as the walls have, brought it to **0.000 m**; with a few points the walls' sampling held it where it was, because the pairs of points on the walls pull towards the nearest point of the wall and the few of the partition could not outvote them. A matcher reports the same rms either way: the rms is small whether the scan constrains the move or not. A scan match is only as good as the geometry it sees, and the geometry is something to check, not assume. See `E-VEH-0065`.

### What does not match

A person who is in one scan and not the other has points with no partner. Left in, they pull the match: it was **0.042 m** out with no limit on the pair distance and **0.000 m** with a limit of 0.3 m, which ignored the points with no partner nearby. The limit is a statement of how far the guess can be from the truth, and should shrink as the match settles. See `E-VEH-0066`.

## Build It

The centroid and rotation step of `icp` in `exercise/solution.hpp`.

```
rcpp verify 25-04
```

## Use It

`rc/locate/icp.hpp` opens nothing new in `rc::locate`.

```cpp
const rc::locate::IcpResult r = rc::locate::icp(new_scan, old_scan, odometry_guess, 50, 0.5, 1e-5);
if (r.converged && r.paired > 100) pose = rc::field::compose(pose, rc::field::inverse(r.move));
```

**What this does not do.** It gives a move and no uncertainty, and a match that has slid along a corridor looks just as sure as one that has not. It pairs points by distance and not by what they are, a point-to-point match; matching to the lines the points lie on converges faster and is less hurt by the spacing of the points. The pairing is quadratic in the number of points, which is fine for hundreds. It takes the scans as taken at one instant (lesson 24-07 undoes that). And it assumes the world is still: a scene that is mostly moving matches itself to itself.

## What Breaks First

- **A first guess too far off.** See `E-VEH-0064`.
- **A corridor, a field, a long wall.** See `E-VEH-0065`.
- **A match with no limit on the pairing distance.** See `E-VEH-0066`.

## Ship It

`rc/locate/icp.hpp` holds `P2`, `IcpResult`, `move_point` and `icp`.
