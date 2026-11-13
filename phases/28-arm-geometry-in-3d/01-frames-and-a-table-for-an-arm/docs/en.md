# Frames and a Table for an Arm in Three Dimensions: Poses, the Two Denavit-Hartenberg Conventions, and Two Real Arms Written Down

> An arm is a table of numbers and a convention for reading it; get the convention wrong and every answer is wrong together.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 13-12, 06-04

## The Problem

The ground course had a vehicle in a plane: three numbers for a pose. An arm lives in **space**. Its tool has a place (three numbers) and an orientation (three more), and the tool is the end of a chain of six or seven joints, each turning about an axis that is not the last one's. To say where the tool is, an arm needs a way of **describing the chain** that anyone can read, and a way of **composing poses** without losing the rotation on the way.

## The Concept

### A pose is a rotation and a translation

A pose in 3D is a rotation matrix (the orientation) and a vector (the place): `Pose`. Composing `a` then `b` means **b is expressed in a**: `compose(a, b)`. The inverse is not the transpose of the whole matrix: the rotation is transposed, and the translation is **turned round by it and negated**. Leaving that out is a classic: a pose 0.5 m from the origin, inverted by transposing the rotation alone, was **0.35 m out**; the correct inverse gave the identity to **0.0000000000000002**. See `E-ARM-0001`.

### A table for the chain

A joint of an arm is described by four numbers (**Denavit and Hartenberg, 1955**): the **link length** `a`, the **link twist** `alpha`, the **link offset** `d` and the **joint angle** `theta`. For a revolute joint `theta` is the variable; for a prismatic one it is `d`. One row turns into a pose by four elementary moves. There are **two conventions** and they put the moves in a different order:

- **Standard:** `Rz(theta) Tz(d) Tx(a) Rx(alpha)`: the frame of a joint is on its **distal** end.
- **Modified (Craig):** `Rx(alpha) Tx(a) Rz(theta) Tz(d)`: the frame is on its **proximal** end.

The same four numbers, read in the two conventions, gave frames **0.170 m** apart. A table is therefore **a claim in a convention**: copying the numbers of one convention into code that reads the other gives an arm that is wrong in a way that looks plausible and that no single check (a joint angle of zero, say) is likely to reveal. See `E-ARM-0002`.

### Two real arms written down

`ur5()` is the **UR5** (standard convention) and `panda()` the **Franka Emika Panda** (modified): the numbers each maker publishes, with the joint limits of the Panda. The first frame of the UR5 sat **0.089159 m** above the base, and its axis was turned a quarter turn, because `alpha` of the first row is a quarter turn: the second joint's axis is horizontal. **Every number here is from the maker's documents and is to be checked against the data sheet of the arm in front of you**; the course has not touched either arm. (The UR5 has six joints and offsets that make a closed form for it hard; the Panda has seven, so it is redundant: both are used in the lessons that follow.)

### A rotation does not stay a rotation

A rotation composed with a small turn a million times is not exactly a rotation any more: rounding drifts the columns from unit length and right angles. Here it drifted **0.000000000126** from orthonormal in double precision (in single precision, it would be a great deal worse); `orthonormalised` (Gram-Schmidt on the columns) put it back to **0.0000000000000002**. A chain of seven poses is not a million, but a controller that integrates an orientation every millisecond is. See `E-ARM-0003`.

## Build It

`inverse` in `exercise/solution.hpp`.

```
rcpp verify 28-01
```

## Use It

`rc/arm/frames.hpp` opens the module `rc::arm`.

```cpp
const rc::arm::ArmTable t = rc::arm::ur5();
const rc::arm::Pose row0 = rc::arm::dh_standard(t.rows[0].a, t.rows[0].alpha, t.rows[0].d, q0);
```

**What this does not do.** It is a rotation **matrix**, not a quaternion: there is no interpolation of orientations here and a matrix of nine numbers carries three that are redundant (the quaternions of 06-02 do not drift in the same way). The tables are for two arms and are **not measured**: an arm off the shelf differs from its table by the tolerance of its manufacture, which the last lesson of this phase estimates. A joint's `theta_offset` is there and does nothing until the next lesson. Units are metres and radians throughout. Nothing checks that a table is a legitimate one (consecutive axes, a right-handed frame): a row can be written that no real joint matches.

## What Breaks First

- **An inverse that forgets the translation.** See `E-ARM-0001`.
- **A table read in the wrong convention.** See `E-ARM-0002`.
- **A rotation that has drifted.** See `E-ARM-0003`.

## Ship It

`rc/arm/frames.hpp` holds `Pose`, `compose`, `inverse`, `apply`, `rot_x`, `rot_y`, `rot_z`, `trans`, `rotation_error`, `orthonormalised`, `dh_standard`, `dh_modified`, `DhConvention`, `JointRow`, `ArmTable`, `kPi`, `ur5` and `panda`.
