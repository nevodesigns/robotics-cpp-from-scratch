id: E-CTRL-0005
title: A waypoint follower that circles instead of arriving
match: expected .*arrived
match: expected .*worst_after_arrival
platforms: linux, windows
teaches: 14-03-closing-the-loop
---

## Symptom

The robot approaches the target, passes it, turns, comes back, passes it again,
and repeats. Or it never gets there at all, tracing a wide arc around the goal.

## Cause

One of two things, and often both.

The controller drives forward at full speed while it is still badly aimed, so it
travels away from the target while turning towards it. For a target behind the
robot this can continue indefinitely, because the arc keeps the target behind.

Or it does not slow down on approach, so it arrives at speed, overshoots the
arrival tolerance, and has to come back. A robot moving at half a metre a second
covers a five centimetre tolerance in a tenth of a second, which is roughly one
control cycle at 10 Hz.

## Fix

Make forward speed depend on how well aimed the robot is, and fade rather than
switch so the command does not step:

```cpp
const double aim = 1.0 - std::min(1.0, std::fabs(error) / turn_first_threshold);
```

Then scale by distance so it settles:

```cpp
const double approach = std::min(1.0, distance_to(pose, target) / slowdown_radius);
const double forward = max_forward * aim * approach;
```

Test it by continuing to drive for several seconds after arrival and requiring
the robot to stay put. A follower that overshoots passes a test that stops the
moment it first arrives.
