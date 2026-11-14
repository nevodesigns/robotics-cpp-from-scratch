# Giving Way on Purpose: Cartesian Impedance for an Arm That Meets a Surface, Admittance for One That Cannot Be Torque Controlled, and Why Each Likes a Different Surface

> To touch a surface the arm has to be a spring, and the stiffness of the spring is the force it will press with.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 29-05, 28-03, 14-07

## The Problem

An arm that is told to go to a point goes to the point, whatever is in the way. That is the right thing in free space and the wrong thing at a surface: a position controller that is 2 mm off a table that is 2 mm too high pushes, as hard as it takes, and an arm pushing that hard breaks something. A task that touches (polishing, inserting, wiping, a hand on a person) needs the arm to give way, and to give way by a known amount: to behave, at its tool, as a spring with a stiffness you choose, so that the force it presses with is that stiffness times how far the target is past the surface.

## The Concept

### Impedance: be a spring

Cartesian impedance control makes the tool push on the world as a spring and a damper would. In each of six directions, the wrench is the stiffness times the pose error minus the damping times the velocity; the joint torques that produce a wrench at the tool are the Jacobian's transpose times it (28-03); and the torque of gravity (29-02) is added so the arm does not sag. One line of the control law, in the space the task is in.

With the target 2 cm below a surface (which pushes back like a stiff spring of 20 000 N/m), a stiffness of 200, 500 and 1000 N/m settled at 4.02, 9.75 and 19.00 N: the stiffness times the 2 cm, to within the surface's own give. The stiffness is the force you will press with. See `E-ARM-0043`.

### The stiffness is also the impact

Coming down on the surface, a stiffness of 200 N/m peaked at 27.6 N and one of 2000 N/m at 68.6 N: a stiff arm is a stiff impact. Compliance is not only for the force you settle to; it is what the arm does in the first moment of contact, before anything has measured anything. See `E-ARM-0044`.

### Admittance: let the force move the arm

Many arms (a hobby arm with position servos, most industrial arms) cannot be torque controlled: their inner loop takes a position. Admittance goes the other way: measure the force at the tool and let it move the position the arm is told to go to, through a virtual mass, damper and spring: `mv * shift'' + dv * shift' + kv * shift = force`. The arm gives way as that virtual system would. With a virtual spring of 500 N/m the force it settled to was 9.3 N (500 times 2 cm).

### Each likes a different surface

The two are not interchangeable. Impedance is a force-out controller: the arm's own dynamics are the loop; it is stable on a stiff surface. Admittance has the measurement and the inner loop's lag inside the loop: on a stiff surface the force changes by 20 N for a millimetre of motion, and a delayed, slow loop turns that into a ring. A damper of 100 N s/m on the stiff surface was 0 (1 steady, 0 not steady); with 1500 N s/m 1; and on a soft surface of 500 N/m even the damper of 100 was 1. The rule: the virtual damper must be at least about the surface's stiffness over the inner loop's bandwidth. See `E-ARM-0045`.

## Build It

`impedance_torque` in `exercise/solution.hpp`.

```
rcpp verify 29-06
```

## Use It

`rc/arm/compliance.hpp`.

```cpp
const double k[6] = {2000, 2000, 500, 10, 10, 10}, d[6] = {60, 60, 90, 0.5, 0.5, 0.5};
const auto tau = rc::arm::impedance_torque(model, q, qd, target, tool, k, d);
```

**What this does not do.** The model must be good for impedance to be what it says: with a wrong model the arm is a spring plus a disturbance (29-05's lesson), and friction (29-04) adds a dead band to the force you press with. The surface here is a plane and a spring; a real surface has friction of its own, a tilt and a shape. The force is not measured: impedance uses the pose error, so the force you press with is a computed figure (a force sensor is for admittance, and for knowing). The admittance simulation is one vertical axis with a mass on a rail; the inner loop's bandwidth and the delay are the author's numbers. There is no contact detection (31-04) and no switching between free motion and contact, which is where most of the trouble is in real work.

## What Breaks First

- **A stiffness that is the force.** See `E-ARM-0043`.
- **A stiff arm hitting a surface.** See `E-ARM-0044`.
- **An admittance on a surface that is too stiff for it.** See `E-ARM-0045`.

## Ship It

`rc/arm/compliance.hpp` holds `Surface`, `contact_force`, `impedance_torque`, `PressResult`, `press`, `AdmittanceConfig` and `admittance_press`.
