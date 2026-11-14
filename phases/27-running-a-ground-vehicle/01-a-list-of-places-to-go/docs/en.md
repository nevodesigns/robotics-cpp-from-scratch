# A List of Places to Go: Waypoints With Tolerances, Dwell and Timeouts, Skipping or Giving Up, and Resuming After a Reboot Without Starting Over

> A mission is not a path, it is a promise about places and what to do when one cannot be kept.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 26-04, 26-08, 21-07

## The Problem

A ground vehicle on a job has a list of places to go: the gate, the pump, the far fence, back to the charger. It is not a path (the planner and the follower of phase 26 make the path between two places) but a promise: be at each of these places, stay a while at some, and say what happened. And a promise has to say what to do when it cannot be kept: the pump is blocked; the vehicle has been rebooted halfway round.

## The Concept

### A leg, a tolerance, a dwell, a timeout

Each waypoint says where, how near counts as there (the tolerance), how long to stand (the dwell), how long the leg may take (the timeout) and how fast to go there. The mission is told the time and the vehicle's position each tick and answers with the place to aim for and a speed. A leg is reached when the vehicle has come within the tolerance and the dwell time has passed (the speed is zero while dwelling). A list of four waypoints was reached in order, standing 4.1 s at the one that asked for it.

### When a leg cannot be done

A leg that has taken longer than its timeout is marked timed out, and the policy decides: skip it and go on to the next, or abort. Here the vehicle could not move on leg 0; with the abort policy the mission ended after 10.0 s, with the skip policy it went on to the next leg and the first one stayed in the record as timed out. Which is right depends on the job: a patrol skips a blocked point, a delivery that cannot reach the customer does not go on to the next one. See `E-VEH-0112`.

### Saving the progress

The mission's progress is a leg number and a result for each leg, small enough to write as text to the flash after every leg. A vehicle that restarts reads it back (`restore`) and goes on from the leg it was on: after a restart at the third leg of four it drove 19.4 m on the rest of the list; one that began again drove 38.4 m, doing the first two legs a second time. The timer of the leg starts again on restore (it is not saved), and text that does not fit the list (a different number of legs, a leg that does not exist) is refused, and leaves the mission at the start. See `E-VEH-0113`.

### A tolerance the vehicle cannot keep

A car cannot be at any point facing any way (lesson 26-01), and a waypoint with a tolerance finer than the car can hit from where it is circles the point and never arrives. A car with a smallest radius of 0.742 m, facing east, with a waypoint 0.8 m to its left (inside its smallest circle, whose nearest pass is 0.68 m from the point) to reach within 0.10 m, was still circling after 40 s (the timeout, and the mission aborted); with a tolerance of 0.70 m it was there in 1.8 s. The tolerance has to be at least what the vehicle can do from where it is: here 0.68 m, the nearest its smallest circle passes the point. See `E-VEH-0114`.

## Build It

`WaypointMission::step` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- If the vehicle is within the tolerance, start the dwell; when the dwell time has passed mark the leg reached and advance; if the leg has taken longer than its timeout mark it timed out, then skip it or abort by the policy.

As shipped, 4 of the 5 tests fail, and these are the ones your code has to turn green:

- A list of waypoints is reached in order, standing at the one that asks to.
- A leg that cannot be reached times out, and the policy skips it or ends the mission.
- Restoring saved progress goes on from the leg, where starting again drives the first legs twice.
- A tolerance finer than the turning circle can be reached keeps a car circling the point.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 27-01
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/drive/waypoint_mission.hpp`.

```cpp
rc::drive::WaypointMission mission(legs, rc::drive::kSkipOnTimeout);
const rc::drive::MissionStatus s = mission.step(now, pose.x, pose.y);
store.write(mission.save());                 // after every leg
```

**What this does not do.** The mission knows nothing about batteries, the link, or whether the vehicle is allowed to go (the next lessons), and the delivery mission of 21-07 is the one that turns back for energy. It does not plan the path to a waypoint (26-01, 26-09), or follow it (26-04): it only says where to aim. A waypoint is a point, not a pose: a dock that needs a heading is a different promise. A leg is reached the first time the vehicle is in the tolerance (a vehicle that passes through at speed counts as having reached it, and a vehicle that needs to stop does so only for the dwell). The save is text with no checksum, and the restore does not know whether the world changed while the vehicle was off.

## What Breaks First

- **A leg that cannot be done and has no policy.** See `E-VEH-0112`.
- **A reboot that starts the mission again.** See `E-VEH-0113`.
- **A tolerance finer than the car can hit.** See `E-VEH-0114`.

## Ship It

`rc/drive/waypoint_mission.hpp` holds `Waypoint`, `LegStatus`, `MissionPolicy`, `MissionStatus` and `WaypointMission`. Phase 27 opens with it.
