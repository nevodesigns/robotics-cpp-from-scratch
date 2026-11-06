# Where the Arm Works Well: Reachable Area, Usable Area and the Joint Limits That Cut Both

> Reachable is where the tool can be put. Usable is where it can be put and then moved. They are different
> areas, and the joint limits cut each of them by less, and differently, than they appear to.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 13-04, 13-05

## The Problem

A cell is being laid out and the arm has to serve a bench. The reach check says every point on the bench is reachable, the bench goes in, and the
arm does the job slowly with its elbow racing, because the bench sits on the edge of the arm's range, where it is nearly straight.

Reachable was the wrong question. The right one is where the arm can do work, and the answer depends on the singularity of lesson 13-04 and on the limits
of lesson 13-05, neither of which a ring of radii shows.

## The Concept

### Reachable is a ring

A two link arm with links of 0.5 and 0.4 m reaches every point between |l1 - l2| = 0.1 m and l1 + l2 = 0.9 m from the shoulder. The area
of that ring is pi times (0.9^2 minus 0.1^2).

`map_workspace` does not use that formula. It walks a grid, asks at each cell whether any legal pose puts the tool there, and adds up the area of the cells that did. The
formula is the check on it: 2.5129 m^2 from 129600 cells, against the ring's 2.5133.

A grid is the method that still works once joint limits make the answer a shape nobody has a formula for.

### Usable is a smaller ring

The determinant of the Jacobian of this arm is l1 times l2 times |sin q2|, so how well the arm can move at a point depends on the elbow and on nothing else
(lesson 13-04). Call a point **usable** when the best legal pose there has at least some fraction of the best possible, which is l1 times l2.

Half of the best means |sin q2| of at least a half, an elbow between 30 and 150 degrees, and that is a ring of its own, with radii from the geometry:

```
r = sqrt(l1^2 + l2^2 + 2 l1 l2 cos q2)    for q2 = 150 and 30 degrees: 0.252 and 0.870 m
```

Its area from the formula is 2.1766 m^2 and the grid gave 2.1763: 86.6 percent of the reachable area.

### The demand sets the area

| needed fraction of the best | usable area (m^2) | percent of reachable |
|---|---|---|
| 0.0 | 2.5129 | 100.0 |
| 0.1 | 2.5016 | 99.6 |
| 0.3 | 2.3984 | 95.4 |
| 0.5 | 2.1763 | 86.6 |
| 0.7 | 1.7950 | 71.4 |
| 0.9 | 1.0963 | 43.6 |

A mild demand costs almost nothing and a strict one costs most of the ring. The edges are thin, and the demand is where the work goes. See `E-KIN-0023`.

### Both elbows count

Every point in the ring has two poses, and a limit can rule out one of them. `best_manipulability` asks `legal_solutions` for both, keeps the better of the legal ones and returns a
negative number only when neither is legal. A map made from one branch reports holes that are not there. See `E-KIN-0024`.

### What a limit costs

| limit | reachable (m^2) | of the free arm |
|---|---|---|
| none | 2.5129 | 100 percent |
| shoulder in the top half only | 1.7591 | 70.0 percent |
| elbow bends one way only | 2.5129 | 100 percent |

The arm on a wall keeps 70 percent and not 50, because its second link folds back and reaches below the shoulder's own line. The one way elbow
loses no area and no usable area: the two poses for any point are mirror images, and one of them is always the legal one. What it loses is the choice between them. See `E-KIN-0025`.

## Build It

`best_manipulability` and `map_workspace` in `exercise/solution.hpp`.

```
rcpp verify 13-10
```

The tests compare the grid against a formula before anything is claimed from it, which is a habit for any number a simulation produces.

## Use It

`rc/kin/workspace.hpp` joins `rc::kin`.

```cpp
const WorkspaceMap m = map_workspace(0.5, 0.4, limits, 0.5, 0.005);
// put the task where the arm is usable, not merely reachable
```

**What this does not do.** It is a planar two link arm, so the manipulability is the closed form l1 times l2 times |sin q2|, and a longer chain has no such
formula (use the Jacobian of 13-03). It says where the tool can be put and not what orientation the tool has there. It ignores obstacles, the arm's own links colliding, and what is
held. And the grid has a resolution: a 5 mm cell cannot show a feature of 2 mm, and the area has an error of about the cell size times the perimeter.

## What Breaks First

- **Reachable taken for usable.** The task sits on the edge. See `E-KIN-0023`.
- **A map built from one elbow.** Holes that are not there. See `E-KIN-0024`.
- **A limit assumed to halve the area.** It costs 30 percent, or nothing. See `E-KIN-0025`.

## Ship It

`rc/kin/workspace.hpp` holds `WorkspaceMap`, `best_manipulability` and `map_workspace`.

Phase 13 so far has said where the tool is, how it gets there, what holds it and what it can push. This says where in the plane to ask it to.
