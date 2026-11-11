# The Table That Describes an Arm: Denavit-Hartenberg Rows, Two Conventions, and the Same Numbers That Are Two Different Arms

> A table of four numbers a joint describes an arm only together with the name of the convention it was written for.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 13-01, 06-04

## The Problem

A vendor sends the description of an arm as a table: four numbers for each joint. A program turns each row into a transform and multiplies them. The tool ends up in the wrong place by a few centimetres, and nothing about the table is wrong. The table was written for the other convention.

## The Concept

### A row is four moves

A row has theta (an angle about the joint axis), d (a distance along it), a (a distance along the common perpendicular) and alpha (a twist). The **standard** convention turns it into `Rz(theta) Tz(d) Tx(a) Rx(alpha)`; the **modified** convention (Craig) into `Rx(alpha) Tx(a) Rz(theta) Tz(d)`. The joint variable is added to theta for a joint that turns and to d for one that slides.

### Checks against what is known

A link of 2 along x turned by 0.6 put the tool at (2 cos 0.6, 2 sin 0.6, 0). Two links of 1 and 0.7 with no twist and no offset were the planar arm: x = 1 cos q1 + 0.7 cos(q1 + q2), and y the same with sine. Composing frames is associative to 1e-12. A row with alpha of a quarter turn, d of 0.5 and a of 0.4 had the rotation `[[c, 0, s], [s, 0, -c], [0, 1, 0]]` and the position (0.4 c, 0.4 s, 0.5), and the rotation was orthonormal. See `E-KIN-0029`.

### A SCARA arm

Two joints that turn and one that slides, the second with a twist of pi so that the slide goes down. Its tool is x = a1 cos q1 + a2 cos(q1 + q2), y likewise with sine, and z = d1 less the slide. Over **243** configurations, **0** were off from that closed form by more than 1e-12. See `E-KIN-0031`.

### One table, two arms

A table of three rows with twists and offsets, and one set of angles: the standard convention put the tool at (1.05255, 0.461061, -0.0389418) and the modified at (1.14953, -0.1, 0.22172), **0.626211** m apart. Even a flat table of two links with no twist at all was **0.244589** m apart: the modified convention puts the length of a row before its angle. With every joint variable at zero and no twist or offset they agree. The lesson of the table is that it is not self-describing. See `E-KIN-0030`.

### The edges

A prismatic joint slid along z by its variable on top of its offset (0.2 and 0.5 gave z = 0.7) and did not turn. A variable that is missing is zero: with only the first angle given, two links of 1 turned a quarter turn were at (0, 2). An offset of the zero of a revolute joint, theta of a quarter turn, put a link of 1 along y at zero. A chain of no joints is the base.

## Build It

`dh_frame` and `dh_chain` in `exercise/solution.hpp`. The elementary moves and `compose` are supplied.

```
rcpp verify 13-12
```

## Use It

`rc/kin/dh.hpp` joins its module.

```cpp
std::vector<rc::kin::DhRow> rows = load_table("arm.dh");        // and the convention is in the file name, or in the datasheet
const rc::kin::Frame tool = rc::kin::dh_chain(rows, joint_angles, rc::kin::Convention::kModified);
```

**What this does not do.** It composes transforms for a table, and does not know which convention a table is in: that has to come with it. It has no inverse, so it says where the tool is for given angles and not which angles put it somewhere, and for most six axis arms the inverse is the hard part and has its own methods. A joint axis that is not along z of its frame, or two parallel axes where the common perpendicular is not unique, are cases in which a table has a choice and two correct tables describe the same arm. And it is in the angles of the table: a joint that reads angles from its own zero needs the offset in theta.

## What Breaks First

- **The four moves multiplied in the wrong order.** See `E-KIN-0029`.
- **A table used in the wrong convention.** See `E-KIN-0030`.
- **A slide taken for a turn, or an offset forgotten.** See `E-KIN-0031`.

## Ship It

`rc/kin/dh.hpp` holds `Frame`, `compose`, the four elementary moves, `DhRow`, `Convention`, `dh_frame` and `dh_chain`.

The next lesson says how wrong a pose is, in position and in orientation.
