# One Speed Budget for Base and Arm: What a Person's Nearness Allows, What the Load Allows, and a Supervisor That Shares the Smaller Between Them

> The person near the robot does not care which part is moving: the speeds of the base and the hand add, and the budget is for both.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 33-08, 33-06, 31-06, 27-03

## The Problem

31-06 gave the speed a person's nearness allows for a robot moving toward them, and 33-05 and 33-06 gave how hard the base may brake with a load. A mobile manipulator is two moving things: the base and the hand. A person standing near it does not care which part is moving: what matters is the speed of whatever is coming at them. The supervisor that holds the robot's speeds has to count the two together, and it has to hold the base to its own limit as well.

## The Concept

### The load's limit on the base

The base may go only as fast as it can stop in the room it has. If it brakes at a fraction (the margin, here 0.8) of the deceleration at which it would tip (5.19 m/s2 with the load, 33-05), then to stop in 0.5 m it may go at most the square root of twice the margin, the deceleration and the distance: 2.04 m/s. A load or an arm reaching out lowers the tipping deceleration and so the speed. See `E-MOB-0025`.

### The person's budget is for both

31-06's `max_speed` is the speed of the robot toward the person. Here the robot's speed toward them is the base's plus the hand's (a hand that swings out as the base drives is faster than either). The supervisor holds the base to its own limit first, then, if the base and the hand together are over the person's total, scales both by the same factor: neither is starved, and the ratio is kept, so the motion keeps its shape. A person 4.0 m away left the speeds as asked (status 0, free); one at 1.0 m allowed 1.23 m/s in all, so the base's 1.0 m/s and the hand's 0.5 m/s were scaled to 0.82 and 0.41; and one at 0.2 m, inside the distance for standing still, stopped both (status 2). Moving away from a person is not limited. See `E-MOB-0026`.

### Against a person who walks at the robot

In a simulation a person walks at 1.6 m/s at a robot that is asked to go toward them. The supervisor sees the separation a reaction time late and the robot slows no faster than it can brake. The supervised robot came to rest 0.14 m from the person: clear of them, but 28 mm inside the 0.17 m of intrusion and uncertainty that the distance was meant to keep. The reason is that a speed limit that falls continuously as the person nears asks the robot to slow faster than its brakes can follow, where 31-06's formula has it at constant speed through the reaction time and then braking: a supervisor that scales speed needs more margin (a larger reaction time or uncertainty in the numbers, or a lower speed asked) than the formula alone. The same robot unsupervised, at the same speeds, met the person, closing on them at 3.1 m/s. See `E-MOB-0027`.

## Build It

`share_budget` in `exercise/solution.hpp`.

```
rcpp verify 33-09
```

## Use It

`rc/mobile/mm_supervisor.hpp`.

```cpp
const rc::mobile::SpeedLimits l = rc::mobile::supervise(ssm, separation, base_ask, hand_ask, tip_brake, 0.8, stop_dist);
```

**What this does not do.** It is one dimensional: the speeds are components toward the person, and a real supervisor takes directions, several people, and every point of the robot (a hand far from the base moves faster when the base turns, 33-04). The robot's response is a bounded deceleration after one reaction time, as in 31-06, with the same typical numbers, and it is a model, not a safety function: a real one is designed against the current standards and measured on the real robot. The load's limit uses the tipping deceleration of one pose, which changes as the arm moves. The interlock of 33-08 (base and arm not moving together) is a separate rule and this supervisor does not replace it.

## What Breaks First

- **A limit on the base that forgets the load.** See `E-MOB-0025`.
- **A person's budget given to the base and again to the hand.** See `E-MOB-0026`.
- **A supervisor that was never tried against someone walking at it.** See `E-MOB-0027`.

## Ship It

`rc/mobile/mm_supervisor.hpp` holds `SpeedLimits`, `tip_speed_limit`, `share_budget` and `supervise`.
