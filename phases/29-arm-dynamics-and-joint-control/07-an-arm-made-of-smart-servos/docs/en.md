# An Arm Made of Smart Servos: A Position Command That Is Quantised, Slow to Arrive and Answered by a Spring, and How to Hold a Pose Anyway

> With a serial bus servo you do not control a torque, you ask a small controller somewhere else for a position, and then you live with its answer.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 29-06, 29-02, 22-01

## The Problem

Phases 28 and the lessons before this one in 29 assumed an arm whose torques you command. A great many arms, among them nearly every hobby and educational arm and a good many research arms, are made of smart servos on a serial bus: each joint is a motor, a gearbox, an encoder and a small controller of its own, and what the computer sends is a position goal, over a wire shared with the other joints. The controller you write is therefore not a torque controller. It asks a small controller somewhere else for a position, and it has to understand how that controller answers.

## The Concept

### The command is rounded

A servo has an encoder with a number of steps to a turn, commonly 4096, so a goal is rounded to a multiple of 0.00153 rad. At 0.7 m from the shaft that is 1.07 mm at the tool, in one joint, before anything else. The rounding error is at most half a step and cannot be smaller. See `E-ARM-0046`.

### The bus has a cycle time

Six servos share one wire. To update them the controller sends one packet with a goal for each, and then reads the position back from each in turn; each servo waits a little (its return delay) before it replies. Ten bit times go to a byte. At 1 Mbit/s the cycle took 2.98 ms for six servos, at 115 200 bit/s 14.3 ms: a rate of 336 Hz at best against 70 Hz; twelve servos at 1 Mbit/s took 5.68 ms. A rate that low is the sampling rate of anything you build on top, and it is set by the wire, not by the processor. See `E-ARM-0047`.

### The servo is a spring

Inside the servo is a position loop with a stiffness (here 15 N m/rad), a damper, a limited torque and a limited speed. It is not a stiff controller: a joint that holds up a load of 2 N m droops by the load over the stiffness, 0.133 rad here, and the servo does not know it has drooped, because its own error is the droop (this is the sag of 29-02, in a servo's own units). The fix is the one of 29-02, in the goal: shift it by the load over the stiffness, using the torque of gravity of the model, and the joint settles where it was wanted: 0.0002 rad off. See `E-ARM-0048`.

### The command is late

A smooth ramp, sent as goals at an update rate, arrives as a staircase and is followed by a spring: the joint runs behind the command. A ramp of 0.5 rad/s sent at 20 Hz was followed 97 ms late; sent at 100 Hz, 77 ms. That lag is part of the loop of anything above it (the delay of 14-07) and shrinks as the rate goes up, which is why the bus matters; it does not go to zero, because the servo's spring and damper have a lag of their own (about the damping over the stiffness, 73 ms here), which is what is left at 100 Hz. And a large step is not followed at once: the servo's speed limit (3 rad/s here) governs, and a step of 1 rad was covered in 0.39 s.

## Build It

`bus_cycle_seconds` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- The write is its overhead plus the bytes for each servo; the read is the request plus, for each servo, its reply and its return delay; each byte takes ten bit times.

As shipped, 1 of the 4 tests fail, and these are the ones your code has to turn green:

- The bus takes a few milliseconds a cycle and a slow bus cannot keep an arm up to date.

The other 3 pass already, because they test the parts that are given.

```
rcpp verify 29-07
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/arm/servo_bus.hpp`.

```cpp
const double cycle = rc::arm::bus_cycle_seconds(bus, 6);            // the fastest update the wire allows
joint.set_goal(rc::arm::goal_for_load(spec, wanted, gravity_torque));
```

**What this does not do.** It is one servo model with typical numbers (a stiffness, a damping, a stall torque, a speed limit and a return delay that are the author's, to be replaced by those of the servo in hand, read from its data sheet or measured). It does not model bus errors (a lost packet, a checksum, a servo that does not answer), which a real bus has and which the code on top has to handle; temperature (a servo that is held against a load for a minute heats and cuts its torque: 29-08); backlash (29-04) or calibration of the zero. The servo's own gains are what the manufacturer set (some servos let you change them: the model's stiffness should follow). The load is a number you give it, from the model of 29-01, and a servo has no torque sensor to tell you if it is right (most report a current or a load percentage, which is a rough torque).

## What Breaks First

- **A command finer than the servo can take.** See `E-ARM-0046`.
- **An update rate the bus cannot give.** See `E-ARM-0047`.
- **A pose held with the servo's own spring.** See `E-ARM-0048`.

## Ship It

`rc/arm/servo_bus.hpp` holds `quantise`, `BusSpec`, `bus_cycle_seconds`, `ServoSpec`, `ServoJoint`, `droop_under_load`, `goal_for_load` and `ramp_lag`.
