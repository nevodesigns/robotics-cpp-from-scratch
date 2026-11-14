# How Much the Motors Can Give: Torque Limits and Heat in an Arm, the Payload That Fits at a Pose, and the Fastest a Move Can Be Made and Repeated

> An arm that can lift a kilogram at the elbow cannot lift it at arm's length, and one that can do a move once cannot necessarily do it all day.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 29-05, 29-02, 27-04

## The Problem

Phase 28 asked where the tool can go and phase 29 so far what torques a move needs. A motor has two limits on torque and neither is the one on the front of the data sheet. The peak torque is what it can give for an instant. The continuous torque is what it can give for ever without getting too hot. Between them is a question of time: the torque needed to hold an arm out is a current, a current is heat, and heat has a clock on it (27-04 did this for a ground vehicle's motor on a slope). For an arm the numbers change with the pose, and so does what it can lift.

## The Concept

### Torque is current is heat

At the output of the joint, a torque `tau` takes a current `tau / kt` through the winding, and the winding heats at `I squared R`. Its temperature rises towards `power times thermal resistance` with a time constant of `thermal resistance times heat capacity`, and the time to a limit has a closed form (as in 27-04): `-Rth Cth ln(1 - limit / steady)`, or never if the steady rise is under the limit.

A joint holding the arm stretched out needed 37.7 N m at the shoulder and (for a motor of 4 N m per ampere at the output, 1 ohm, with a thermal resistance of 1.2 K/W and a heat capacity of 200 J/K, all typical) reached its limit in 293 s. With the arm folded it needed 2.1 N m and never reached it. The same arm, the same motor, a few degrees apart in the pose: holding a pose stretched out is something an arm can do for minutes and not for hours, and nothing in the control code says so. The closed form agreed with stepping the lump to a few percent. See `E-ARM-0049`.

### The payload that fits

The torque to hold a pose is the arm's own (29-01) plus the payload's weight through the Jacobian (29-02). It is linear in the one unknown, the mass, so for each joint `|own + mass * per_kg| <= limit` is an interval of masses, and the answer is where all the intervals overlap. With the continuous limits and a rating of 10 kg for the structure, the arm could hold 2.50 kg at the tool with the arm stretched and 10.00 kg (the full rating) with it folded: the rated payload of an arm is the figure at the worst pose, and the one in the data sheet is usually for the best. An arm that cannot hold itself at the pose gives a negative answer. See `E-ARM-0050`.

### The fastest a move can be, and be repeated

A move needs its torque at every instant: the peak must be inside the peak limit, and the root mean square (which is what heats) inside the continuous limit. A move of 0.7 rad could be done in 0.29 s if only the peak mattered and in 0.39 s by the continuous limit. The part of the torque that is the inertia goes as the inverse square of the duration: halving the time of the move asked 4.0 times the torque of its inertia part (the weight does not change).

And a move that is fine once is not fine all day. Repeated with no wait, a move raised the shoulder's winding by a steady 118 K, over the limit of 75; with 4 s of holding the end pose between, 27 K; with 30 s it was well under. The cycle time of a pick and place is partly set by the heat. See `E-ARM-0051`.

## Build It

`max_payload` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- For each joint |own + mass times per_kg| <= limit is an interval for the mass (two ends, the smaller and the larger of the two solutions); the answer is where all intervals overlap, from zero up.

As shipped, 2 of the 5 tests fail, and these are the ones your code has to turn green:

- The payload that fits depends on the pose, and a payload at the limit is held at the limit.
- A payload that the arm alone cannot hold is refused with a negative answer.

The other 3 pass already, because they test the parts that are given.

```
rcpp verify 29-08
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/arm/limits_heat.hpp`.

```cpp
const double m = rc::arm::max_payload(model, q, tool, continuous_limits, rated_payload);
const double t = rc::arm::shortest_move_time(model, move, motors, false);
```

**What this does not do.** The motor constants are typical, with a thermal model of one lump (no fan, no sun, no temperature of the air, no gearbox heating) and a continuous torque that is a number you give it. The payload is assumed at the tool and the arm at rest: lifting while moving asks more. The torques of a move are from the model of 29-01 (rods) and have no friction: a real arm's friction adds to the heat at low torque (29-04). The shortest move time is for one move, with the same duration for every joint, not the time-optimal path of 30. A joint's speed limit (a motor's back voltage, 29-07's servo limit) is not in it; a fast move can be limited by speed and not by torque. The duty is the rms heat over one cycle, which is the right measure for a winding with a long time constant and the wrong one for a short thermal path in a drive's electronics.

## What Breaks First

- **A pose that heats the motor to its limit in minutes.** See `E-ARM-0049`.
- **A payload rating taken from the best pose.** See `E-ARM-0050`.
- **A move that is fine once and not all day.** See `E-ARM-0051`.

## Ship It

`rc/arm/limits_heat.hpp` holds `JointMotor`, `heat_watts`, `time_to_limit`, `max_payload`, `MoveTorques`, `move_torques`, `within_motors`, `shortest_move_time` and `duty_rise`. With it, phase 29 is complete: an arm that is modelled with its mass, held against gravity, driven by a cascade whose gains know the pose, compensated for friction and play, steered by computed torque or by impedance, through a servo bus, within the torque and heat its motors have.
