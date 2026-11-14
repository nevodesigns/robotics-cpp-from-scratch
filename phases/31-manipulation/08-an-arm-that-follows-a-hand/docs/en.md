# An Arm That Follows a Hand: Mapping a Leader's Joints to a Follower's, Following Within Its Limits, and What Delay and a Lost Link Do

> The follower must do what the leader does and never more than it safely can; the gap between those is the whole design.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 31-07, 29-07, 31-06

## The Problem

Some tasks are not planned but led: a person moves a leader arm (a pair of hobby arms, a leader and a follower, is the commonest case) and the follower arm does the same. Phase 31's last lessons record such demonstrations. The idea is simple and its failures are not: the follower must do what the leader does and never more than it can safely do, and between those lie delay, a message rate, a jump, and a lost link.

## The Concept

### From a leader's reading to a follower's target

Joint for joint: the leader's reading less its zero, times a scale, plus the follower's zero, clamped to the follower's limits. Two arms of the same make still have different zeros (how the horns were put on the servos), and the zeros are calibrated (a pose both arms are put in). A wrong zero is a constant offset between what the person does and what the follower does. See `E-ARM-0097`.

### The follower follows within its limits

A follower joint goes toward the target as a first order lag (a velocity of the error over a time constant tau), clamped to a speed limit. The lag is what makes the motion smooth; the clamp is what makes it safe. A jump of 1.0 rad in the leader (a hand that slipped, a bad message) was followed at no more than 2.0 rad/s, taking 0.48 s, not in one tick. See `E-ARM-0098`.

### What the link adds

A follower with a time constant of 50 ms following a leader moving as 0.5 sin(pi t) rad had an rms error of 0.059 rad with the leader read at 100 Hz, 0.081 rad at 20 Hz (the target is held between messages), and 0.168 rad at 100 Hz with 100 ms of delay. Rate and delay both add to the error, and delay makes a person overshoot, since what they see lags what they did. Both are numbers to measure on a real link (29-07 gave a bus's).

### A lost link

When no message has come for a timeout, the follower should hold. The leader jumped by 1 rad and the last message was sent a moment later. A follower with a watchdog moved 0.000 rad after the timeout (it held where it was, which is true by construction and is the point); one without it went 0.398 rad on toward the last message, which was the leader's place a moment ago and not now. See `E-ARM-0099`.

## Build It

`follower_step` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- Ask for a velocity of (target - q) / tau, clamp it to plus or minus vmax, and move by that times dt.

As shipped, 4 of the 5 tests fail, and these are the ones your code has to turn green:

- A follower step goes toward the target as a lag and never faster than its limit.
- Slower messages and delay both add to the tracking error.
- A jump in the leader is followed at the speed limit, and no faster.
- When the link is lost a follower with a watchdog holds, and one without goes on to the last message.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 31-08
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/arm/teleop.hpp`.

```cpp
const double target = rc::arm::map_joint(joint_map, leader_reading);
if (rc::arm::link_alive(now, last_rx, 0.2)) q = rc::arm::follower_step(q, target, 0.05, vmax, dt);
```

**What this does not do.** There is no force back to the leader (no haptics), no collision checking of what the follower does (phase 30's planner is not in the loop: a person leading an arm into the table is followed into the table, so the contact detection of 31-04 and the limits of 29-08 are what protect it), and the joints are independent: the two arms are taken to be of the same kind. The leader's message is a single number per joint at a fixed rate; the real thing has a bus (29-07), a clock, and dropped messages that are not modelled here.

## What Breaks First

- **A zero that was not calibrated.** See `E-ARM-0097`.
- **A follower with no speed limit.** See `E-ARM-0098`.
- **A link that dies with the follower still moving.** See `E-ARM-0099`.

## Ship It

`rc/arm/teleop.hpp` holds `JointMap`, `map_joint`, `follower_step` and `link_alive`.
