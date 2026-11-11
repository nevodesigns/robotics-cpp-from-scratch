# The Door That Closed: Noticing That the Map Is Out of Date Before the Map Notices

> The map was right when it was made. The door has opinions of its own.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 16-13, 16-05

## The Problem

A map is a record of what the room was like. Lesson 16-05 gave it three answers and a way to take in new readings. A door closes, a pallet is left in a corridor, a chair is moved, and the plan that was made through the doorway leads straight into a wall.

An occupancy grid does eventually take the new wall in, but slowly: it was built to average out noise, and a cell that has been free for a long time does not become a wall on one observation. In the meantime the robot plans through the door. The rover needs to know sooner that the map is wrong, without
mistaking every bit of noise for a changed room.

## The Concept

### Three kinds of beam

Lay a scan on the map from the robot's pose. Each beam that hit something tells one of three things:

- **Agree:** it ended on a wall of the map, or beside one.
- **Appeared:** it ended in free space, with the cell and its eight neighbours all known free. There is a wall here that the map does not have.
- **Vanished:** on its way it passed through a wall that the map does have, more than two cells short of the end. The wall is not there.

A beam that ends among unknown cells says nothing about the map being stale, and one that found nothing counts only if it crossed a wall. A scan of the map as it is gave 82 agreeing, 0 appeared and 0 vanished.

A door that had closed, with the robot believing it open:

| | agree | appeared | vanished |
|---|---|---|---|
| believed open, now closed | 82 | 9 | 0 |
| believed closed, now open | 80 | 0 | 11 |

The two directions give different counts: a wall that has appeared and one that has vanished are both changes, and a robot that can only tell one of them misses half the world. See `E-NAV-0041`.

### Noise is not change

Real ranges are noisy. A scan of the room as it is, with 3 cm of noise on each range, still has a few beams that end a cell away from where the map says. Call a scan different if more than a threshold share of its judged beams disagree. 250 scans of the room unchanged, and 250 after the door closed:

| threshold | unchanged scans called different | closed-door scans called different |
|---|---|---|
| 0.01 | 0.800 | 1.000 |
| 0.02 | 0.484 | 1.000 |
| 0.04 | 0.072 | 1.000 |
| 0.08 | 0.004 | 1.000 |
| 0.15 | 0.000 | 0.012 |

At 0.01 most unchanged scans are called a change. At 0.15 the door is missed altogether. Between them, 0.04 to 0.08, noise rarely fools it and the door is always seen. See `E-NAV-0042`.

### Wanting it to last

A person walks past, and for one scan a quarter of the beams end at them. That is a change in the scan and not in the map. A **detector** wants the disagreement to persist: more than the threshold on several scans in a row, three here. One odd scan starts a run and the next ordinary one ends it, and a scan that
judged nothing neither adds to the run nor ends it. See `E-NAV-0042`.

### The map you compare with

Here is the part that is easy to get wrong. The robot is also repairing its map from every scan. Compare each scan against the map as it is being repaired, and the repair destroys the evidence.

| | noticed at scan |
|---|---|
| compared with the map as it was | 3 |
| compared with the map being repaired | never |

After a single scan the doorway cells have had one hit each against two misses, which is no longer free: they are unknown, and a beam ending at an unknown cell is not an appearance. The disagreement disappears from the count before the third scan, the run never reaches three, and the detector never fires, even though the repaired map's doorway
cell did not become a wall until scan 11. Compare against a copy of the map made before the repair, and keep it until the change has been decided. See `E-NAV-0043`.

## Build It

`compare_scan` and `ChangeDetector::update` in `exercise/solution.hpp`. The types and `occupied_near` are given.

```
rcpp verify 16-15
```

The scans are made by marching rays through the true world in the test, which does not use the code under test, and the noise comes from a seeded generator, so the figures above are the same on every run.

## Use It

`rc/nav/change.hpp` joins `rc::nav`.

```cpp
ChangeDetector detector(0.05, 3);
// every scan, comparing with the map as it was before anything was repaired:
if (detector.update(compare_scan(reference, beams, 6.0, pose))) replan_with(fresh_map);
```

**What this does not do.** It takes the pose to be right. A pose that is wrong makes every wall look moved, which is lesson 16-13's problem, and a detector that fires on a bad pose is telling the truth about the wrong thing. It cannot tell a door from a person standing in a doorway, and it does not say what changed, only that something did. It looks at the ends of beams and so
does not see an obstacle that is shorter than the beam spacing at that range. The threshold and the run length are set from the noise of the sensor in use, with the table above as the method.

## What Breaks First

- **A scan compared without telling new from gone.** Half the changes are missed. See `E-NAV-0041`.
- **A threshold set by feel.** Noise called change, or a door never seen. See `E-NAV-0042`.
- **Comparing with the map being repaired.** The evidence is erased by the repair. See `E-NAV-0043`.

## Ship It

`rc/nav/change.hpp` holds `Surprise`, `occupied_near`, `compare_scan` and `ChangeDetector`.

The rover now knows when its map is stale, and the map can wait.
