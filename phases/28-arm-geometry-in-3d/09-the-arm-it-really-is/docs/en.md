# The Arm It Really Is: Calibrating the Table From Measured Tool Positions, What a Measurement Cannot Tell Apart, and How Many Poses Are Enough

> The table is the arm that was drawn; the arm on the bench differs from it by a millimetre here and a third of a degree there, and every one of them is in the tool.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 28-05, 28-08, 23-08

## The Problem

The table of 28-01 is the arm as it was **drawn**. The arm on the bench was made: links cut to a length within a tolerance, joint zeros set by an encoder mounted by hand, axes that are parallel to a third of a degree and not exactly. Each error is small and **every one of them is in the tool**: a third of a degree at the shoulder, 0.8 m from the tool, is 4.6 mm. An arm that is told to go to a point goes to a point a few millimetres from it, and a few millimetres is the whole of the budget of an assembly task. **Calibration** finds the table that the arm really has.

## The Concept

### Measure, then fit

Put the tool through a set of poses and **measure where it is** with something better than the arm (a laser tracker, a calibrated camera, a touch probe at a known point). Each pose gives the joint angles and a measured place. The **correction** to the table is four numbers for each joint (the offset of the joint angle, the offset along its axis, the link length and the twist): 24 for a UR5. Find the correction for which the table's predictions are closest to the measurements. That is a nonlinear least squares problem and the standard method is **Levenberg-Marquardt**: find how the tool's place changes with each correction (the Jacobian of 28-03's cousin, by central differences), solve the normal equations with a little damping, and **keep the step only if the error falls**, raising the damping if it does not. (The first version of this lesson added every step, which is plain Gauss-Newton: it passed with 60 poses and 1 mm of noise at 12 iterations and, run for 25, diverged to metres. The accept and reject rule is the whole difference.)

A UR5 whose table was off by up to 0.3 degrees in angles and 1 mm in lengths (the kind of thing manufacture does; **simulated** here, no real arm was measured) put its tool **3.39 mm** from where the table said, rms. Fitted to **60** poses measured with 0.05 mm of noise, the error was **0.081 mm** on those and **0.100 mm** on **30** poses it had not seen: down to the size of the noise. See `E-ARM-0025`.

### The test is the poses it did not see

A fit with fewer measurements than corrections can match them **exactly**. Fitted to only **4** poses (3 equations each, 12 in all, for 24 corrections) the error on its own poses was **0.00000000 mm**, and on others **0.83 mm**: it was fitting the poses and not the arm. With 8 poses the error on others was **0.151 mm**, with 24 **0.107 mm** and with 60 **0.101 mm**: every pose added helps, and the error on poses kept back is the only honest number. Measurement noise matters too: with 1 mm of noise the error on new poses was **0.426 mm**. See `E-ARM-0026`.

### What measuring cannot tell apart

Some corrections give **exactly the same** tool place for every joint angle. The axes of joints 2 and 3 of the UR5 are parallel, so shifting the elbow along one of them or the other is the same shift. Moving the offset along joint 2's axis by 1 mm and joint 3's by 1 mm gave tool places that differed by **0.000000000000 m**. The data **cannot say which**, so the fitted corrections are not the true ones, and **it does not matter** for the tool's place (which is all the calibration promises) and it **does** matter if someone then reads a link length off the result. The damping in the normal equations is what lets the solve go through; a plain solve of the same system is singular. See `E-ARM-0027`.

## Build It

The step of `calibrate` in `exercise/solution.hpp`.

```
rcpp verify 28-09
```

## Use It

`rc/arm/calibrate_kin.hpp`.

```cpp
const rc::arm::CalibResult r = rc::arm::calibrate(nominal_table, measurements, 1e-9, 12);
const rc::arm::ArmTable real = rc::arm::perturbed(nominal_table, r.correction);
```

**What this does not do.** The measurements are **simulated**: a real calibration has a tracker whose frame is not the arm's base frame, and the six numbers for that frame must be fitted too (or the fit absorbs them into the first joint's corrections); the **tool offset** is another unknown. It measures **position only**: orientation is not constrained, so errors that turn the tool without moving it are not seen. It assumes the joint angles are right (the encoders are not wrong beyond their offsets), the arm is rigid (no **sag** under its own weight and a payload, which a calibration at one load does not take away) and nothing changes with temperature. The poses are random, not **chosen** to make the fit well conditioned. There is no check that the correction is small enough to be believed. And the dependent corrections (above) are left as the damping chooses.

## What Breaks First

- **An arm used as it was drawn.** See `E-ARM-0025`.
- **A calibration judged on its own poses.** See `E-ARM-0026`.
- **Corrections that cannot be told apart, read as true.** See `E-ARM-0027`.

## Ship It

`rc/arm/calibrate_kin.hpp` holds `perturbed`, `Measure`, `Lcg3`, `simulate_measurements`, `rms_error`, `solve_dense`, `CalibResult` and `calibrate`. With it, phase 28 is complete: an arm in three dimensions described by a table, moved forward and backward by closed form and by iteration, with its singularities, limits, reach and calibration measured.
