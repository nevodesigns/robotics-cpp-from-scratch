# Straight in Space or Straight in the Joints: Two Ways to Move and What Each Costs

> Moving the joints in a straight line moves the tool in a curve. Moving the tool in a
> straight line needs the inverse kinematics at every point and costs joint speed.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 13-04, 13-05

## The Problem

Lesson 13-02 found the joint angles that put the tool at a point, and lesson 13-05 found
which of them the arm can hold. To move the tool from one point to another there are two
obvious ways, and they are not the same move.

Send each joint from where it is to where it is going, in a straight line in angle, so they
arrive together. Or move the tool in a straight line and work out, at each point along it,
what the joints must be. A weld seam, a pick from a tray and a wipe across a surface want the
second. A transfer from one place to another, through clear air, wants the first, and uses it
because it is cheaper, though most people take it for the second.

## The Concept

### A straight line in angle is a curve in space

The tool's position is a trigonometric function of the angles, so a straight line through the
angles is not a straight line through the tool. The ends are exactly right, which is why a
check of the start and the finish passes it.

How far the tool strays from the straight line between the ends, for a 0.5 m and 0.4 m arm,
five moves of two seconds each:

| move | length | the tool strays |
|---|---|---|
| short, near the middle | 0.320 m | 22.7 mm |
| across the workspace | 0.700 m | 83.6 mm |
| near full reach | 0.500 m | 37.3 mm |
| through the middle | 0.800 m | 121.1 mm |
| radially out to full reach | 0.345 m | 12.7 mm |

The worst is a seventh of the move's length. Even the shortest is off by more than two
centimetres. `path_deviation` measures it as the largest perpendicular distance of the tool from
the line, over a path of joint poses. See `E-KIN-0020`.

### The tool move, one point at a time

`cartesian_path` takes `steps + 1` points along the line and asks the solver of lesson 13-02 for
the joint angles at each. Two things have to be right.

**One branch, kept.** Lesson 13-02 found two answers at almost every point, elbow up and elbow
down. A path takes one and holds it. A solver that picks whichever it likes at each point can
change from one to the other between two neighbours, which the arm cannot do. The test checks
that the elbow never changes sign and that no joint jumps by more than 0.3 radians between
neighbours.

**It can fail halfway.** The workspace of a two link arm is a ring, not a disc: the arm cannot
reach beyond the sum of its links, and it cannot reach inside the difference, a hole round the
base. Two points in the ring can have a straight line between them that passes through the hole.

A line from (0.30, 0.05) to (-0.30, 0.05), both reachable, enters the hole at point 72 of 200.
The path stops there: it reports `complete = false` and where, with the poses before and none
after, because the points beyond may be reachable again and the arm cannot get from one side to
the other. A joint move between those same two ends does exist, and it strays from the straight line by more than 0.05
m. See `E-KIN-0021`.

### The price of a straight tool

A joint move is a constant rate for each joint, which makes it the move with the lowest peak joint
speed for those ends. A tool move needs at least as much, and often far more.

The fastest any joint has to turn to make the same move in two seconds:

| move | joints straight | tool straight | ratio |
|---|---|---|---|
| short, near the middle | 0.27 rad/s | 0.31 rad/s | 1.1 |
| across the workspace | 0.51 | 0.87 | 1.7 |
| near full reach | 0.30 | 0.53 | 1.8 |
| through the middle | 0.59 | 0.83 | 1.4 |
| radially out to full reach | 0.82 | 3.39 | **4.2** |

Away from the edge the cost is modest. Moving the tool radially towards full extension is moving
into the singularity of lesson 13-04: the tool goes at an even speed and the elbow has to turn faster
and faster to keep it going. 3.39 rad/s for a tool going at 0.17 m/s. See `E-KIN-0022`.

A straight Cartesian move near full extension is therefore always a slow one, and the thing that slows
is the tool, to stay inside what the joints can do. `peak_joint_speed` is how a planner finds out, and
the turn between two poses is measured the short way round: a joint going from 3.1 to -3.1 has turned
0.08 radians, and not 6.2.

## Build It

`lerp_joints`, `cartesian_path`, `path_deviation` and `peak_joint_speed` in `exercise/solution.hpp`. The
solver from lesson 13-02 is `rc::kin::solve`. The comments say how each is built.

```
rcpp verify 13-09
```

The tests measure the move with the tool's position computed from the geometry, so a wrong inverse
kinematics, a branch that flips or a deviation measured the wrong way shows up in the tables.

## Use It

`rc/kin/path.hpp` joins `rc::kin`.

```cpp
const CartesianPath p = rc::kin::cartesian_path(0.5, 0.4, x0, y0, x1, y1, 200, rc::kin::Branch::ElbowDown);
if (!p.complete) replan_around_the_base(p.first_unreachable);
const double fastest = rc::kin::peak_joint_speed(p.poses, duration);
if (fastest > joint_limit) duration *= fastest / joint_limit;
```

**What this does not do.** It moves at an even rate in time along the path, with no acceleration at
the start and the end: lesson 14-06's profile is what to run along it. It does not check the joint limits
of lesson 13-05 along the way, and a path that passes through a pose the arm cannot hold is
only found out by checking each pose. It does not choose the branch for you, which is a decision about
the world. And it is two joints in a plane: for six, the same structure holds, with a Jacobian of six
rows and a singularity that is harder to see.

## What Breaks First

- **Joint moves taken for straight lines.** The tool curves by centimetres. See `E-KIN-0020`.
- **A line between two reachable points.** It can pass through the hole. See `E-KIN-0021`.
- **A straight line near full reach.** It needs joint speeds that grow without much warning. See
  `E-KIN-0022`.

## Ship It

`rc/kin/path.hpp` holds `JointPose`, `Branch`, `lerp_joints`, `CartesianPath`, `cartesian_path`,
`path_deviation` and `peak_joint_speed`.

Phase 13 now has the arm's geometry, its limits, its dynamics, its forces, and the two ways it can be moved.
