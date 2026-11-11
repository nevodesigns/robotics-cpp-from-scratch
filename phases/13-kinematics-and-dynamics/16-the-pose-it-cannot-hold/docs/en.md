# The Pose It Cannot Hold: Asking an Arm Its Question in Both Directions, and Telling the Pose It Cannot Reach From the One It Cannot Take or Hold

> Three different reasons an arm cannot do a pose have three different remedies. Say which, and for a pose that can be done, choose the way and say how long it takes.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 13-15, 13-14, 13-13, 13-11, 13-06, 13-05

## The Problem

An operator asks an arm to go to a pose and is told that it failed. There are three reasons, and they are not the same: no angles put the tool there at all, or angles exist and a joint cannot turn to them, or the angles are legal and the motors cannot hold the arm there. The remedies are different: move the target, or reach it another way, or use a lighter payload. A planner that says only "failed" gives the operator nothing to change.

## The Concept

### The question in both directions

Forward kinematics says where the tool is for angles; inverse says which angles put it somewhere. Neither says whether the arm can do it. `plan_move` asks for a target pose and says which of four things is true, in this order: **unreachable** (the wrist cannot be reached), **outside the limits** (neither way of reaching it has every angle inside its limit), **too heavy** (neither of the legal ways is one the motors can hold), or **ok**.

### What it takes to hold

`holding_torques3` is gravity times, for each joint, the masses beyond it times their horizontal distance from it. With links of 1, 1, 1, masses 2, 1.5, 1 and a payload of 0.5, straight out along x: **71.1225**, **31.8825** and **9.81** N m. Straight up it is nothing, and straight down it is nothing and not a negative number. Folded back it is less at the shoulder, and a payload of 2 kg raised the wrist torque to 9.81 times 2.5.

### Four answers

| the target | the answer |
|---|---|
| straight out, payload 0.5 kg | **ok**, at **0.889031** of the limit of the shoulder, 0 s from there |
| (2.9, 0) pointing back | **unreachable** |
| (1.5, 0.5) with an elbow that may bend 0.5 rad | **outside the limits** (it needs about 2.55) |
| straight out, payload 1 kg | **too heavy**, at **1.07297** of the limit |

The reason is the answer. See `E-KIN-0041`.

### The heaviest payload

By bisection over the payload, for the pose straight out, the heaviest the arm can hold is **0.801648** kg. At that payload the shoulder is at **80** of 80 N m, the elbow at 37.8008 of 40 and the wrist at 12.7692 of 15: it is the shoulder that gives out first. A little below was ok and a little above was too heavy. See `E-KIN-0042`.

### Which of the two ways

For the pose (2.2, 0.3, 1.2) the two ways of reaching it asked **0.765126** and **0.732123** of the limits of the motors. Starting near one way, the plan takes that way: the quicker of the two. Tighten every limit to between them, so that one can be held and the other cannot, and starting next to the harder way the plan still takes the easier. (For some poses the two ways ask the same: the wrist motor holds the tool, whose heading is the same either way, and a lesson that looked at only one such pose would not see the choice.)

### An ok plan

From (0.2, -0.4, 0.3) to the pose (1.8, 0.7, 0.5): the tool ended on the pose to 1e-9, in position and heading, every angle inside its limit, and the move took **1.21006** s, the time of the slowest joint. From where the arm already is, it took none. See `E-KIN-0043`.

## Build It

`holding_torques3` and `plan_move` in `exercise/solution.hpp`. The specification of the arm and the verdicts are supplied.

```
rcpp verify 13-16
```

## Use It

`rc/kin/arm_planner.hpp` joins its module.

```cpp
rc::kin::ArmSpec arm;  arm.payload = 0.8;
const rc::kin::Plan p = rc::kin::plan_move(arm, current, {1.8, 0.7, 0.5});
switch (p.verdict) {
  case rc::kin::Verdict::kUnreachable:   say("that point is out of reach");              break;
  case rc::kin::Verdict::kOutsideLimits: say("a joint cannot turn that far");           break;
  case rc::kin::Verdict::kTooHeavy:      say("the motors cannot hold that payload there"); break;
  case rc::kin::Verdict::kOk:            start_move(p.solution, p.duration);            break;
}
```

**What this does not do.** It is a static check: the torque to hold the arm still, and not the torque to move it (lesson 13-07), so a move that is held at both ends can still ask more of a motor on the way, and a payload that a pose can hold may not be one it can carry through a fast move. It is the planar arm, with a vertical plane and links that are uniform bars. It checks the pose at the end and not the path to it, which can pass a limit or a singularity in between (lessons 13-04 and 13-09). It chooses by the time of the move and does not weigh the torque, so that two ways that are both holdable are not compared by how near the limit they leave the motors. And the numbers of the arm are those of this lesson: a real arm has to have its own measured.

## What Breaks First

- **A failure with no reason.** See `E-KIN-0041`.
- **A payload limit nobody worked out.** See `E-KIN-0042`.
- **A way chosen without looking at what the motors can hold, or at the time.** See `E-KIN-0043`.

## Ship It

`rc/kin/arm_planner.hpp` holds `ArmSpec`, `Verdict`, `Plan`, `holding_torques3` and `plan_move`.

This is what phase 13 was for: an arm asked its question in both directions, and told apart the pose it cannot reach from the one it cannot hold. Phase 14 is where the arm is made to follow what it has planned.
