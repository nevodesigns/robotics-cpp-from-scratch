# Where the Tool Points: Position and Heading of a Three-Link Arm, and the Target That Is Reachable as a Point and Not as a Pose

> A third link gives the tool a heading. A target is then a pose, and the test of whether the arm can do it is the test of the wrist.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 13-02, 13-05

## The Problem

A two-link arm puts its tool at a point and cannot say which way it points. Add a third link and the tool has a heading too: a gripper has to approach a part from above, a camera has to look along a wall. A target is now a position and a direction, and an arm can fail to reach a pose that is within reach of its tool.

## The Concept

### Forward

`forward3` gives the position of the tool and its heading, which is the sum of the three angles, each measured from the link before it. With links of 1, 1 and 1: all angles 0 gave (3, 0) pointing along x; 90 degrees, then -90, then 0 gave (2, 1); a heading is not wrapped by the sum, so folding the arm out, back and out gave a heading of 2 pi.

### The short way round

A heading of 3.14 and one of -3.14 are almost the same direction. Their difference as numbers is **6.28**; the short way round, `heading_error`, is **-0.00318531**. A controller that steers on the plain difference turns a full circle to correct an error of a fifth of a degree. The error is always in (-pi, pi]. See `E-KIN-0026`.

### Inverse

`solve3` finds the wrist, which is the tool less the last link laid along the heading, reaches it as a two-link arm does (two ways, elbow up and elbow down), and sets the third angle to turn the heading into the one asked for. Over a grid of poses, **2498** solutions were checked and **0** were off by more than 1e-9, in position or in heading.

### A point is not a pose

With links of 1, 1 and 1 the total reach is 3, and the point (2.9, 0) is inside it. Pointing along x, the wrist is at 1.9, which the first two links reach. Pointing back along x, the wrist would be at 3.9: **not reachable**, `TooFar`. A point near the base is fine; one beyond 3 is out in any heading; and with links 2, 1 and 0.5 the tool at the origin pointing along x puts the wrist at (-0.5, 0), inside the hole the shorter link of the first two cannot reach into. See `E-KIN-0027`.

### How many headings

At (1.5, 0) with links 1, 1 and 1, **209** of 360 whole-degree headings could be reached: the wrist must be within 2 of the base, which is cos(phi) at least -0.25. At (2.0, 0) there were fewer. The further out, the fewer the headings. See `E-KIN-0028`.

## Build It

`forward3`, `heading_error` and `solve3` in `exercise/solution.hpp`.

```
rcpp verify 13-11
```

## Use It

`rc/kin/arm3.hpp` joins its module.

```cpp
const rc::kin::Pose2 target{1.2, 0.8, 1.57};
const auto s = rc::kin::solve3(target, 0.5, 0.4, 0.2);
if (!s) { /* the pose is out of reach: say so, with s.error() */ }
else command(s.value().elbow_up);
```

**What this does not do.** It is an arm in a plane, with the pose of three numbers, and a real arm is in space with six, which the next lessons build towards. It gives the two solutions and does not choose between them, or check them against joint limits (lesson 13-05), and an arm with a third link that is not needed for the heading has a freedom that this solves by fixing the heading. The angles come out wrapped, so a joint that can turn through more than a turn is not asked for the turn it would take. And it says too far or too close for the wrist, not which of position and heading was the cause.

## What Breaks First

- **A heading difference taken as a number.** See `E-KIN-0026`.
- **A pose refused or accepted by the position alone.** See `E-KIN-0027`.
- **A solution that does not put the tool on the pose.** See `E-KIN-0028`.

## Ship It

`rc/kin/arm3.hpp` holds `Pose2`, `Arm3Solution`, `Arm3Solutions`, `forward3`, `heading_error` and `solve3`.

The lessons that follow give the arm a pose in space, a rule for how wrong a pose is, and a time to get there.
