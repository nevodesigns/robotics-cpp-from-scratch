# A Differential Drive That Slips: Wheel Speeds to Motion, With the Two Losses a Real Floor Adds, and What Each Costs in Distance and in Turn

> A wheel that turns is not a robot that moves by the same amount, and the difference is the first thing odometry gets wrong.

**Type:** Build
**Time:** about 75 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-03, 22-05, 22-12, 21-10

## The Problem

The model of lesson 00-05 says that two wheels at known speeds move a robot by an amount that follows from the geometry. A wheel on a floor does not know that. The floor gives way a little under every metre of rolling, so the robot goes less far than the wheels went, and when one wheel is turned against the other the tyres have to scrub sideways across the ground to make the body rotate, so it turns less than the arithmetic says. These are two different losses, they have different causes, they have different effects, and a model with neither is a model of a robot on a floor that does not exist.

## The Concept

### Two losses

**Longitudinal slip** takes a fraction of the wheel travel: the ground speed is the mean wheel speed times the radius times (1 minus the slip). Over a wheel travel of 10 m a slip of 5 percent left the robot **9.50 m** from where it began. It is in the direction of travel, so on a straight line it is only a scale error, and a straight line is the case where it matters least.

**Turn efficiency** takes a fraction of every turn: the body's turn rate is the efficiency times the wheel difference over the track width. At an efficiency of 0.8 a pair of wheel speeds that the arithmetic says is a quarter turn gave **72.0 degrees**. To make the quarter turn the wheels must go a quarter further (**25 percent more**), and with the model of lesson 00-05 a controller that asks for a quarter turn gets 72 degrees and is surprised. See `E-VEH-0001`.

### The integration

`DiffSlip` takes the two wheel speeds in rad/s and moves a pose. It uses the heading at the **middle** of the step, which was the lesson of the hardening pass: along an arc, starting from the heading at the start of the step adds an error that grows with the step. On a quarter circle of radius 0.4 m (wheels at 20 and 30 rad/s) it ended at (0.4, 0.4) at 90 degrees within 2 mm.

### A square that does not close

The test of every ground robot: drive 10 m, turn a quarter, four times, and see whether it comes home. On the commands of the model with no loss, the ideal model closed within 0.023 m (the time is cut into steps of a millisecond). With a slip of 2 percent and an efficiency of 0.9, the robot ended **4.64 m** from the start. The two losses are not the same size, and the efficiency is the one that does the damage here, because every turn that is 10 percent short leaves the next side pointing the wrong way, and the error grows with the length of the side. See `E-VEH-0002`.

### Reading the loss from a run

The efficiency is not on a data sheet; it depends on the floor, the tyres and the weight. But it can be read from a turn: the wheels travelled so far each way and the body turned so much, and the ratio of the turn to the turn the wheels ask for is the efficiency. A turn measured with a model whose efficiency was 0.8 read back as **0.800**. The next lessons do this on the other kinds of vehicle, and a later lesson of this phase calibrates it from a run with noise. See `E-VEH-0003`.

## Build It

`DiffSlip::step` in `exercise/solution.hpp`.

```
rcpp verify 23-01
```

## Use It

`rc/vehicle/diff_slip.hpp` opens the module `rc::vehicle`.

```cpp
rc::vehicle::DiffSlip model(0.033, 0.160, 0.02, 0.9);
model.step(wl, wr, 0.02);
const rc::vehicle::Pose2 p = model.pose();
```

**What this does not do.** Both losses are constants, and on a real floor they are not: slip grows with the force on the wheel, and the efficiency with the weight, the turning radius and the surface. The model has no sideways slip, so it cannot show a robot drifting out of a fast bend, and no dynamics, so wheel speeds act at once. The wheel radius and the track are taken as exact. The next lessons of this phase add the surface, the actuators and a calibration.

## What Breaks First

- **A turn asked of the arithmetic and not of the floor.** See `E-VEH-0001`.
- **Odometry that trusts the wheels.** See `E-VEH-0002`.
- **A loss assumed and not measured.** See `E-VEH-0003`.

## Ship It

`rc/vehicle/diff_slip.hpp` holds `Pose2`, `DiffSlip` and `turn_efficiency_from`.
