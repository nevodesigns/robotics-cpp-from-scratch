# Closing the Loop: Driving the Robot to a Target

> Four lessons, four small pieces, and this is the one where they become a robot that goes where it is told.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none, the robot is simulated
**Prerequisites:** 14-02, 00-04, 01-03

## The Problem

You have all the parts and none of the machine.

Lesson 00-04 gave you a model that turns wheel speeds into motion. Lesson 01-03
gave you the shortest turn between two headings. Lesson 14-01 gave you a
controller that drives an error to zero without winding up. Lesson 14-02 gave you
a watchdog that stops the machine when commands go stale.

Point at a spot on the floor and none of that gets the robot there. Something has
to decide, sixty or a hundred times a second, what the wheels should do, and that
something is what you build now.

## The Concept

### Going backwards through the model

The model in `rc::sim` answers: given wheel speeds, how does the robot move?

```
forward = (left + right) / 2
turn    = (right - left) / wheel_base
```

A controller needs the opposite. It decides on a forward speed and a turn rate,
and needs the wheel speeds that produce them. Rearranging the same two equations:

```
left  = forward - turn * wheel_base / 2
right = forward + turn * wheel_base / 2
```

That is the **inverse kinematics** of a differential drive, and it is genuinely
this small. One of the tests checks the round trip: ask for a forward speed and a
turn rate, convert to wheels, run those wheels through `rc::sim::step`, and the
robot must move exactly as asked. If the two disagree, one of them is wrong, and
the test does not care which.

### What the controller controls

The obvious idea is to control position directly, and it does not work: a
differential drive cannot move sideways, so a controller commanding x and y asks
for motion the machine cannot perform.

What it can do is turn to face the target and drive forward. So the error being
controlled is the **heading error**: the angle between where the robot faces and
where the target lies.

```cpp
const double desired = std::atan2(target.y - pose.y, target.x - pose.x);
const double error = rc::sim::wrap_angle(desired - pose.theta);
```

The wrap is not optional. Without it, a target slightly behind the robot produces
an error of nearly a full turn, and the robot dutifully spins most of the way
round the long way. That is `E-NUM-0006`, and lesson 01-03 exists because of it.

### Do not drive while badly aimed

A controller that always drives forward at full speed while turning traces a long
arc away from the target before it comes round, and for a target directly behind
it can circle indefinitely.

The fix is to make forward speed depend on how well aimed the robot is. Not as a
switch, which produces a step change in the command and is exactly what the rate
limiter in lesson 14-02 exists to prevent, but as a fade:

```cpp
const double aim = 1.0 - std::min(1.0, std::fabs(error) / turn_first_threshold);
```

Badly aimed gives zero, aimed gives one, and in between it eases. The robot turns
almost in place when it needs to and drives cleanly when it can.

### Slow down on approach

A robot at full speed arriving at a point half a wheel base wide will overshoot,
notice, turn around, and overshoot the other way. It looks exactly like an
untuned controller because it is one.

Scaling forward speed by distance, up to a limit, makes it settle:

```cpp
const double approach = std::min(1.0, distance_to(pose, target) / 0.4);
```

One of the tests keeps driving for many seconds after arrival and requires the
robot to stay put, which a version without this fails.

### Why the watchdog guards the target

The target does not come from inside this class. It comes from a planner, an
operator, or a network, and any of those can stop.

So `set_target` feeds the watchdog, and `command` refuses to move when it has
expired. A target is perishable, and a robot driving confidently towards a goal
that nobody has confirmed for two seconds is the shape of a real accident.

The check comes **first**, before any control arithmetic, because the safe state
should not depend on the rest of the function being correct.

## Build It

Implement in `exercise/solution.hpp`:

- `heading_error(pose, target)`, wrapped.
- `to_wheel_speeds(forward, turn, wheel_base)`, the inverse of the phase 00 model.
- `WaypointDriver::set_target`, which feeds the watchdog.
- `WaypointDriver::command`, in the order given in the comments: watchdog, then
  arrival, then heading error into the PID, then forward speed, then wheel speeds
  clamped to the limit.

```
rcpp verify 14-03
```

The tests drive the simulated robot to targets in every direction, including
directly behind, and check that it arrives, stays, and never asks a wheel for
more than it has.

## Use It

`nav2` in ROS 2 does this with a pluggable local controller, and the default one
is a descendant of the same idea with obstacle avoidance and a velocity window on
top. The structure you have written, an error, a controller, a limit and a
watchdog, is the structure underneath.

What is missing here is everything about the world: obstacles, a map, other
robots, and the fact that the robot does not truly know where it is. Phase 16
adds estimation and planning. This lesson is the layer they all sit on.

## What Breaks First

- **The robot spins the long way round for a target behind it.** The heading
  error was not wrapped. See `E-NUM-0006`.
- **The robot moves before any target has been set.** A watchdog that has never
  been fed must count as expired. See `E-CTRL-0003`.
- **The robot circles the target forever without arriving.** It drives forward
  while badly aimed, or does not slow on approach. See `E-CTRL-0005`.

## Ship It

`WaypointDriver` joins `rc::control`, and it is the first thing in this
curriculum assembled from several phases at once: the model from phase 00, the
angle handling from phase 01, and the controller and watchdog from earlier in
this phase. Phase 16 gives it a planner to take targets from, and the Qt view
from phase 10 draws where it went.
