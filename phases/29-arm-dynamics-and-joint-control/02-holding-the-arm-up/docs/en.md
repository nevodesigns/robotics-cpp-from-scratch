# Holding the Arm Up: Gravity Compensation, the Droop of a Plain PD Controller, and Finding Out What Is in the Gripper

> A controller that has to hold the arm up with its error has already lost some of it.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 29-01, 28-03, 14-04

## The Problem

An arm that is told to stand at a pose has to hold itself up, and a plain joint controller (a gain on the position error and a gain on the velocity) holds it up with its error: the arm sags until the error times the gain equals the weight. The shoulder of an arm stretched out carries tens of newton metres. At a gain of 300 N m/rad that is a tenth of a radian: several centimetres at the tool. Raising the gain helps and makes the controller stiff and noisy; the better answer is to not make the controller do the job.

## The Concept

### Add the weight, in advance

The model of 29-01 gives the torque of gravity for any pose, `g(q)`. Add it to the controller's output, as a feedforward, and the controller is left with only the error: the arm stands still where it is told to. With a gain of 300 N m/rad at the shoulder, the arm under a PD controller alone ended 0.1113 rad from its target, and the torque of gravity over the gain said 0.1093: the droop is the weight over the gain, to within the pose changing as it sags. With the torque of gravity added the error was 0.000000000 rad: the arm is where it was told. See `E-ARM-0031`.

### When the model is wrong: the payload

The model knows the arm. It does not know what is in the gripper. A payload of 2 kg at the tool, that the controller did not know about, left an error of 0.0646 rad: the compensation covered the arm, and the 2 kg was still the controller's to hold, at its gain. This is the usual case, and the cure is the same: add what is missing to the model. See `E-ARM-0032`.

### Weigh what is in the gripper

The extra torque to hold a payload at the tool is the Jacobian's transpose times its weight (28-03: the force gearbox). It is linear in the one unknown, the mass, so a handful of measurements settle it by least squares: hold the arm still in six poses, take the torque the motors needed (or the currents), subtract what the arm alone needs, and find the mass that best explains what is left. From six poses, with 0.1 N m of noise on each torque, the mass was estimated as 2.00 kg (it was 2); put in the model, the error was 0.00010 rad. With nothing in the gripper the estimate is zero. (A payload that is not at the tool, or an unknown centre of mass, has more unknowns and needs more poses and a richer pose set.) See `E-ARM-0033`.

### How the payload goes into the model

A payload is a point mass on the last link. The mass is the sum, the centre of mass is the weighted mean of the two centres, and the inertia is the two inertias about the new centre by the parallel axis rule. Getting it wrong (adding the mass and leaving the centre where it was) puts the weight in the wrong place and makes the compensation wrong in a way that looks like a smaller payload.

## Build It

`pd_torque` in `exercise/solution.hpp`.

```
rcpp verify 29-02
```

## Use It

`rc/arm/gravity_comp.hpp`.

```cpp
const auto tau = rc::arm::pd_torque(model, q, qd, target, gains, true);
const double mass = rc::arm::estimate_payload(table, tool, poses_held_still, extra_torques);
```

**What this does not do.** The compensation is only as good as the model: the typical rod masses of 29-01 are not a real arm's, and a real arm's are measured or taken from the maker. Friction is not in it and holds the arm a little off in either direction (29-04). The simulation is for the arm sagging slowly to rest, not for a motion. The payload estimate is a mass at the tool; another centre, a payload that swings, or a payload picked up halfway through a move are not covered. The PD gains here are the author's, not tuned for a real drive (29-03). The torque of gravity is worked out with the arm's measured pose each cycle and a real controller has a delay on it.

## What Breaks First

- **A controller that holds the arm up with its error.** See `E-ARM-0031`.
- **A compensation that does not know the payload.** See `E-ARM-0032`.
- **A payload added to the mass and not to the centre.** See `E-ARM-0033`.

## Ship It

`rc/arm/gravity_comp.hpp` holds `PdGains`, `pd_torque`, `with_payload`, `payload_torque`, `estimate_payload` and `settle`.
