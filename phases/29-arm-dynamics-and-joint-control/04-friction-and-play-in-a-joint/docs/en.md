# Friction and Play in a Joint: Stick and Slip, a Gear With a Gap in It, and Which Encoder the Loop Should Watch

> A joint is not a mass on a rail; it sticks, it slips, and when it reverses it spends a moment going nowhere.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 29-03, 14-08, 22-04

## The Problem

The model of 29-01 and the cascade of 29-03 treat a joint as a mass that a motor pushes. A real joint is a motor, a **gearbox** and a load, and two things in it are not like a mass. **Friction**: it opposes motion, and is larger when the joint is still than when it is moving. **Play**: a gear is made with a small gap in it, and when the motor reverses it travels the gap before it touches the load. Both are small, and both are exactly the size of the errors an arm is then asked to make good.

## The Concept

### Friction is not one number

A torque that opposes the motion has three parts: a **Coulomb** part (the same at any speed, the sign of the velocity times a constant), a **viscous** part (in proportion to the speed), and, at the lowest speeds, a **Stribeck** part that is added to the Coulomb and falls away as the joint starts to slide: it takes more to **start** a joint than to **keep it going**. With 0.05 N m of Coulomb and 0.02 of Stribeck, the torque to resist motion **peaked at 0.0559 N m at 0.017 rad/s**, fell to **0.0503 N m at 0.05 rad/s** (past the static peak and before the viscous part has grown) and was **0.0550 N m** at 1 rad/s. The sign must be **smoothed** over a small speed (here 0.01 rad/s): a bare sign chatters at zero. See `E-ARM-0037`.

### Feed it forward

A joint that is to creep at 0.03 rad/s has to be given the friction torque for that speed, or the controller has to supply it from its error. A controller with an integral supplies it **late** (the integral has to wind up to it, and then the joint breaks free and overshoots: stick and slip). Adding the friction torque for the **speed the reference asks for** takes the work away from the error: following a ramp of 0.03 rad/s the load was **0.00000 rad** off on average, and **0.00000 rad** with the friction fed forward. The feedforward goes with the **reference** velocity, not the measured one: the measured one is noise near zero. See `E-ARM-0038`.

### Play, and finding it

A gear with a gap has a **dead zone**: the motor can move across it with no effect on the load. Push the motor with a torque above the friction and watch the load: the **motor's travel before the load moves** is the play. The true gap of **0.0100 rad** (2 times the half gap) was measured as **0.0102**. A gear with no gap measured nearly zero.

### Which encoder to close the position loop on

An arm with an encoder on the **load** and one on the **motor** has a choice, and play makes it matter. (The velocity loop stays on the motor, as a drive's does.) Closing the **position** loop on the **load** puts the dead zone **inside** the loop; with an integral on the position error, so that the loop can hold a target to the last bit, the integral builds while nothing happens in the gap, the load jumps, and it **hunts**. Holding a target the load moved **0.0013 rad** over the last second and ended **0.0006 rad** off. Closing it on the **motor** has no dead zone inside it: the loop is steady (the load moved **0.000000 rad**) but it controls the motor, so the load sits somewhere in the gap, **0.0034 rad** off the target (about half the gap). The right answer for accuracy is the load encoder with the play handled (approach every target from the same side, so the gear is always loaded the same way, or keep the integral small or give it a deadband); the right answer for a smooth loop is the motor. (Without the integral on position the load loop did not hunt at all in this model: the integral is the part that does it.) See `E-ARM-0039`.

## Build It

`friction_torque` in `exercise/solution.hpp`.

```
rcpp verify 29-04
```

## Use It

`rc/arm/joint_friction.hpp`.

```cpp
rc::arm::JointController c;  c.friction_ff = true;  c.feedback_load = false;
const double tau = c.torque(joint, q_ref, v_ref, 1e-3);
```

**What this does not do.** The friction is one model for one joint, and a real joint's friction depends on **temperature**, on the **load** and on the **position** (a bad bearing has a ripple). It does not **identify** the friction: the numbers are given, and a real one is measured by driving the joint at several constant speeds and plotting torque against speed. The gap is a **dead zone with a stiff contact**: a real gear has an elastic gearbox (harmonic drives wind up like a spring) which is another effect. The feedforward uses the **believed** friction equal to the true one; a wrong one has its own error. The simulation is a **two mass** model with one joint: the other joints and gravity are not in it. No **cure** for the hunting is given beyond the choice of encoder (a deadband, a lower integral gain, approaching from one side are all used).

## What Breaks First

- **Friction as a single number.** See `E-ARM-0037`.
- **A slow move that sticks and jumps.** See `E-ARM-0038`.
- **A loop on the load with play in it.** See `E-ARM-0039`.

## Ship It

`rc/arm/joint_friction.hpp` holds `FrictionModel`, `friction_torque`, `TwoMass`, `JointController`, `RampResult`, `track_ramp`, `HoldResult`, `hold` and `measure_play`.
