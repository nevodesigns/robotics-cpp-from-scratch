# What the Arm Weighs and How It Swings: Rigid Body Dynamics of a Six Joint Arm by Recursive Newton and Euler, the Mass Matrix, and an Energy That Must Not Drift

> Every torque an arm needs is one of three things: holding it up, speeding it up, or the way the joints throw each other about.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 28-03, 13-07, 13-06

## The Problem

Phase 28 was geometry: where the tool is for given joint angles and how it moves when they do. An arm also has mass. A torque is what a motor gives, and what a motor must give depends on what the arm weighs, how fast it is being swung and how the joints throw each other about. For a controller that is to move an arm well, and for a motor that is to be chosen big enough, the arm needs a dynamic model: given a motion, the torques; given torques, the motion.

## The Concept

### Three kinds of torque

Every joint torque of an arm is the sum of three things:

```
tau = M(q) qdd   +   C(q, qd) qd   +   g(q)
      speeding up    the joints throw   holding it up
                     each other about
```

- Gravity `g(q)`: what it takes to hold the arm still, and it depends on the pose (the arm stretched out costs the shoulder far more than the arm folded).
- Inertia `M(q) qdd`: what it takes to speed it up. `M` is a matrix: accelerating one joint takes torque at the others, and it changes with the pose.
- Velocity products `C qd`: the Coriolis and centripetal terms, which come from one joint's motion acting on another's. They grow with the square of the speed.

### Recursive Newton and Euler

The method that gives all three for an arm of any number of joints, in a time proportional to the number: recursive Newton and Euler. Forward along the chain, work out for each link its angular velocity, its angular acceleration and the acceleration of its centre of mass (each from the one before and the joint's rate and acceleration). That gives, by Newton and Euler's equations, the force and moment each link needs. Backward from the tool, each link must be given that force and moment plus what it passes on to the next: add them up, and the part of the moment about the joint's axis is the joint's torque.

Link by link it needs the mass, the centre of mass and the inertia of each. Here each link is a uniform rod along the line between joint frames, with a mass and a radius (the numbers are typical, for a small arm: 3, 4, 2, 2, 1 and 0.5 kg). The mass matrix has the torque for a unit acceleration of each joint, with no velocity and no gravity, as its columns; the velocity products are the torques with velocity and no acceleration and no gravity; gravity is the torque with no motion.

### Checked against things that are true

- **By hand:** a rod of 2 kg and 0.4 m out horizontally from a joint with a horizontal axis needs 3.924 N m to hold it, which is `m g L / 2`; standing up it needs 0.000000 N m; at any angle, `m g (L/2) cos(q)`.
- **Gravity is the slope of the potential energy:** the torque of gravity matched the numerical slope of `m g z` to 0.00000001 N m over the poses tried. This is the check a sign error in the recursion cannot survive.
- The mass matrix is symmetric (to 0.000000000000) and positive: for all 60 random vectors `x`, `x transpose M x > 0` (kinetic energy is never negative). A mass matrix that is not is a mistake, not a feature.
- Velocity products go as the square of the speed: doubling the joint speeds quadrupled them, to 0.000000000000.

See `E-ARM-0028`.

### Energy: the integrator is part of the model

An arm let fall from rest, with no torque and no friction, must keep its energy. Stepped a millisecond at a time for a second, a fourth order step (Runge and Kutta) left the energy out by 0.000000000496 of itself; a first order step (Euler's) by 0.03723. The same model with a bad integrator is a model that gains or loses energy, and a controller tuned against it learns the wrong lesson. See `E-ARM-0029`.

### Held at gravity, it does not move

Applying exactly the gravity torque as the motor torque gives zero acceleration: the model agrees with itself. That is the gravity compensation of the next lesson.

## Build It

The backward recursion of `inverse_dynamics` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- The force on link i is its own plus what it passes on to the next; the moment about its frame is its own, plus the moment of its weight and inertia force about the frame, plus the next's moment, plus the next force at the next frame's offset; the torque is the part along the joint axis of the moment about the joint's point, which adds the force at the frame's offset from that point.

As shipped, 4 of the 5 tests fail, and these are the ones your code has to turn green:

- A rod out horizontally from a horizontal axis needs m g L / 2 to be held, and nothing when it stands up.
- The torque of gravity is the slope of the potential energy, and the mass matrix is symmetric and positive.
- What the joints throw at each other grows with the square of the speed.
- An arm let fall keeps its energy with a fourth order step and does not with a first order one.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 29-01
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/arm/dynamics3d.hpp`.

```cpp
rc::arm::ArmModel model;  model.table = table;  model.links = rc::arm::rod_links(table, masses, 0.04);
const auto tau_hold = rc::arm::gravity_torques(model, q);
const auto qdd = rc::arm::forward_dynamics(model, q, qd, tau);
```

**What this does not do.** The links are uniform rods with typical masses: a real arm's links are hollow, with motors and cables in them, and their inertias are measured, not assumed (the tables of the UR5 and the Panda do not come with them here). Friction is not in it (29-04). The links are rigid: no flexing, no play in the gearboxes (which are the gearbox's reflected inertia too: a motor with a 100:1 gear is seen at the joint as 10 000 times its own inertia, and that is not added here). It is for arms in the standard convention and, for forward dynamics, six joints. It does not use a payload: a mass on the tool is an extra link, and 29-02 adds it. The speed of the recursion has not been measured.

## What Breaks First

- **A dynamic model with a sign wrong in it.** See `E-ARM-0028`.
- **A model that gains or loses energy.** See `E-ARM-0029`.
- **A mass matrix that is not one.** See `E-ARM-0030`.

## Ship It

`rc/arm/dynamics3d.hpp` holds `LinkInertia`, `ArmModel`, `cross`, `rod_links`, `inverse_dynamics`, `mass_matrix`, `velocity_torques`, `gravity_torques`, `forward_dynamics`, `energy` and `step_dynamics`. Phase 29 opens with it.
