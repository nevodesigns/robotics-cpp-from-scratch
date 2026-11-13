# The Tool in Three Dimensions: Forward Kinematics of a Six and a Seven Joint Arm, the Tool Offset, and Roll Pitch Yaw With Its Hole at Ninety Degrees

> Forward kinematics is multiplication; everything that goes wrong with it is a convention.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 28-01, 13-01

## The Problem

Lesson 28-01 wrote an arm down as a table. The question an arm is asked most often is the simplest: **given the joint angles, where is the tool?** That is **forward kinematics**. It has one right answer for a given table, and nothing makes it hard except the conventions around it.

## The Concept

### Multiply the rows

The tool is the **base**, times the pose of **each row** for its joint angle (the angle plus the row's offset), times a **tool offset**. That is the whole of `forward`. The frame after each joint (`link_frames`) is the same product stopped early, and it is what a drawing, a collision model (phase 30) and a Jacobian (next lesson) all need.

### Checked against the table, not against itself

A test that computes the tool with the code and compares it with the code proves nothing. With every joint at zero the UR5's tool is where the **sums of the table's numbers** put it, read off by hand: `x = a2 + a3`, `y = -(d4 + d6)`, `z = d1 - d5`. It was at (**-0.81725**, **-0.19145**, **-0.00549**) m, which is exactly those. The Panda's flange (with a flange offset of 0.107 m as the tool offset) was at (**0.088**, **-0.000**, **0.926**): its three vertical links, less the flange, and the 0.088 m of its last offset to the side.

### The zero pose may not be a pose

The Panda's **zero pose is outside its joint limits**: the fourth joint may not be at zero (its interval is about -3.07 to -0.07 rad). `within_limits` said **0**. A table is a description of the geometry; the limits say which of its poses the arm can take, and "all zeros" is the pose people reach for first. See `E-ARM-0004`.

### What has to be true of a good one

Turning the first joint turns the whole arm about the base axis: the tool of a turned arm was where the same pose rotated about the base's z axis puts it, to **0.000000000000 m**, and its rotation the same. A joint offset is the same as turning the joint by it. The tool offset moved the tool **0.150 m** along the **flange's own axis**, not the base's: a gripper's tip is not the flange, and forgetting the 0.15 m puts every pick that far from where it should be. See `E-ARM-0005`.

### Roll, pitch and yaw, and their hole

A rotation is nine numbers; people want three. **Roll about x, pitch about y, yaw about z, applied in the order z, y, x** is the usual choice, and it has a **hole**: where the pitch is a quarter turn, roll and yaw turn about the same axis and only their difference means anything. Close to it, a rotation that has hardly moved gives a **jump in roll** (here **57** degrees of roll for a tool 0.0001 rad of pitch away). The code returns the rotation exactly even there, because at the hole it sets the yaw to zero and gives the rest to the roll; but any control that works on the three angles (a limit, a filter, an interpolation) misbehaves near it. See `E-ARM-0006`.

## Build It

`forward` in `exercise/solution.hpp`.

```
rcpp verify 28-02
```

## Use It

`rc/arm/fk.hpp`.

```cpp
const rc::arm::ArmTable t = rc::arm::ur5();
const rc::arm::Pose tool = rc::arm::forward(t, q, base, rc::arm::trans(0, 0, 0.15));
const rc::arm::Rpy e = rc::arm::rpy_from_rotation(tool);
```

**What this does not do.** Every joint is revolute: a prismatic one (a SCARA's, in the last phase of this course) is not read. The geometry is **ideal**: no link is bent, no joint has play, nothing sags under load, and the arm is exactly its table (28-09 is about the difference). The base is a pose you give; where it is in a cell is for you. The Euler angles are one of twelve conventions and the lesson picks one: another vendor's roll is not this roll. The limits are on joint angles; **self-collision** and the limits of the **cables** are not in them.

## What Breaks First

- **A zero pose that is not allowed.** See `E-ARM-0004`.
- **A tool offset left out.** See `E-ARM-0005`.
- **Euler angles near their hole.** See `E-ARM-0006`.

## Ship It

`rc/arm/fk.hpp` holds `row_pose`, `link_frames`, `forward`, `within_limits`, `Rpy`, `rpy_from_rotation` and `rotation_from_rpy`.
