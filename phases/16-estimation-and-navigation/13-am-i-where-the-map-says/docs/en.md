# Am I Where the Map Says: Scoring a Scan Against a Map, and the Corridor That Cannot Tell

> Odometry drifts. The map does not. The scan is the only thing that sees both.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 16-05, 16-09

## The Problem

Lesson 16-01 showed how odometry drifts, and lesson 16-09 calibrated it, which slows the drift without stopping it. A rover with a map has something odometry lacks: places that are where they were. A range finder gives a scan of the walls round it, and the question is where the rover would have to be for that scan to be what the map says it should see.

Answering it means trying poses and scoring them. The score is easy. What the score cannot tell you is the lesson.

## The Concept

### Scoring a pose

Put the scan on the map from a candidate pose: each beam ends at the pose plus its range along its angle plus the pose's heading. If the pose is right, the ends land on walls. The score is the share of beams, of those that found anything, that end on an occupied cell:

```cpp
ex = pose.x + range * cos(pose.theta + angle);
ey = pose.y + range * sin(pose.theta + angle);
```

A beam that found nothing, a range of zero or one off the scale, is left out and not counted as a miss: it says nothing about where the walls are. A perfect scan scored 1.000 from where it was taken, and as the pose moves along x:

| offset (m) | score |
|---|---|
| 0.00 | 1.000 |
| 0.05 | 0.787 |
| 0.10 | 0.656 |
| 0.20 | 0.639 |
| 0.40 | 0.607 |

It falls, and then plateaus at 0.6: a room is mostly walls, and a scan that is shifted still has many ends on some wall. The score is a measure of fit, and a score of 0.6 does not mean the pose is 60 percent right. See `E-NAV-0035`.

### Noise and tolerance

A real range finder is noisy. With 3 cm on each range, at the true pose:

| tolerance (cells) | score |
|---|---|
| 0 | 0.492 |
| 1 | 0.951 |
| 2 | 0.984 |

Asking for the end to land on exactly an occupied cell loses half the beams to noise. Accepting a cell with an occupied neighbour within a cell or two loses almost none. The cost is sharpness, which is why the tolerance is not simply made large. See `E-NAV-0035`.

### A search round the guess

Try every pose in a box round the odometry's guess, in steps, and keep the best. A guess drifted to (1.40, 0.85, 0.40) against a truth of (1.20, 1.00, 0.30) came back as (1.20, 1.00, 0.30) with a score of 1.000, the position error brought from 0.250 m to 0.000 m, with one pose scoring that well. The box is the odometry's drift: too small and the truth is outside it, too large and the search takes longer and finds more look-alikes. See `E-NAV-0036`.

### When many poses fit

The search counts how many poses scored within 0.005 of the best, and that count is the answer to whether the pose was determined at all. A 30 m corridor, 1 m wide, with the ends too far to see:

| | best score | poses tied |
|---|---|---|
| corridor | 1.000 | 41 |
| room with a block against one wall | 1.000 | 1 |

Every position along the corridor fits the scan equally well: nothing in it says where along it you are, and the scan constrains only the sideways position and the heading. A tied pose should not be settled by chance. Of poses with the same score the search takes the one nearest the guess, so a rover in a corridor stays where odometry put it (15.00 against a truth of 15.00 here)
and does not jump. See `E-NAV-0036`.

### When two places look alike

An empty rectangular room is the same seen from either end. The true pose and its mirror through the middle of the room, rotated by a half turn, both scored 1.000. With a block against one wall:

| | true | mirror |
|---|---|---|
| empty room | 1.000 | 1.000 |
| room with a block | 1.000 | 0.869 |

The block gives only a modest difference, because most of a room's walls look alike from anywhere: scoring finds that two poses fit, and the map has to have something to choose between them. See `E-NAV-0037`.

## Build It

`score_pose` and `search_pose` in `exercise/solution.hpp`. The beam and result types and `near_occupied` are given.

```
rcpp verify 16-13
```

The scans are made by marching rays through the map in the test, which does not use the code under test, and the noise comes from a seeded generator, so the figures above are the same on every run.

## Use It

`rc/nav/match.hpp` joins `rc::nav`.

```cpp
const MatchResult m = search_pose(map, scan, 6.0, odometry_pose, 0.3, 0.05, 0.2, 0.05, 1);
if (m.tied == 1 && m.score > 0.9) correct_odometry(m.best);
```

**What this does not do.** It tries every pose in a box, which grows with the cube of the box's size and with the number of beams: a real matcher is smarter about it. It scores with a tolerance and not by distance to the nearest wall, so near misses and far misses score the same, which makes the score coarse.
It trusts the map, and a map with a door open that was closed when it was made scores wrong everywhere (a later lesson in this phase deals with that). It says whether a pose is determined, and not what to do about a rover that is not: more scan from somewhere else, or a different sensor.

## What Breaks First

- **Counting beams that found nothing as misses.** A scan with half its beams out to the horizon scores low. See `E-NAV-0035`.
- **A tolerance of zero.** Noise loses half the beams. See `E-NAV-0035`.
- **A tie settled by chance.** A rover in a corridor jumps by metres. See `E-NAV-0036`.
- **Two poses taken as one.** A symmetric room, the wrong end. See `E-NAV-0037`.

## Ship It

`rc/nav/match.hpp` holds `Beam`, `Candidate`, `near_occupied`, `score_pose`, `MatchResult` and `search_pose`.

The rover can now say where the map thinks it is, and how sure that answer is.
