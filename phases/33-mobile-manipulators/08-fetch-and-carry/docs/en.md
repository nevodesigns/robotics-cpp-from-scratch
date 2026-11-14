# Fetch and Carry: A Mission From a Shelf to a Table, With the Rule That the Base Moves Only When the Arm Is Tucked and the Arm Moves Only When the Base Is Still

> A mission is a short list of steps with a check after each, and the interlock between base and arm is what keeps the robot upright.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 33-07, 33-05, 31-02, 21-07

## The Problem

21-07 delivered something by driving a list of places, and 31-02 picked and placed with an arm that stayed where it was. A fetch and carry joins them: drive to a shelf, take a part, drive to a table, put it down. It is two machines that must not work against each other, because a base driving with its arm stuck out tips (33-05), and an arm reaching while the base creeps misses (33-07). The mission is a state machine like 31-02's, and the interlock between the two halves is its most important rule.

## The Concept

### The steps

Check the battery (it must have 1.2 times the energy the mission needs, 33-06's range); drive to the pick station with the arm tucked; deploy the arm above the part, descend, close, verify by the gap between the fingers that the part is held (31-02's `holding`); lift, stow the arm, drive to the place station, deploy, lower, release, retreat, stow and done. A mission that went to plan took 97 ticks. See `E-MOB-0022`.

### Faults say why

A missed grasp backs off and tries again, up to a limit: a hand that missed once took 113 ticks, and one that always missed stopped in fault 1 (no part) after 3 tries. A part that drops while the base is driving is seen at once from the gap and stops the machine in fault 2. A battery of 4.5 Wh for a mission needing 4.0 Wh stopped it at the start, in fault 4, rather than halfway to the table. Every step has a time limit (fault 3). See `E-MOB-0023`.

### The interlock

Every command the machine gives goes through `guard`: the base may be told to drive only when the arm is tucked, and the arm to go out or down only when the base has stopped (tucking is always allowed). This is a separate, simple rule applied to whatever the state machine says, and it holds even when the state machine is wrong. A faulty plan that tried to drive with the arm out (here a plan that skipped the stow step) was held back and the mission ended in a timeout (fault 3) with 0 unsafe ticks; without the interlock the same plan drove with the arm out for 15 ticks. A bug became a stopped robot instead of a tipped one. See `E-MOB-0024`.

## Build It

`guard` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- If the command drives the base (drive_to is not 0) while the arm is not tucked (arm_at is not 1), cancel the driving; if it sends the arm out or down (arm_goal over 1) while the base has not stopped, hold the arm (arm_goal 0); set *blocked if either was done.

As shipped, 2 of the 4 tests fail, and these are the ones your code has to turn green:

- The interlock holds the base while the arm is out and the arm while the base moves.
- A dropped part, a low battery and a faulty plan are each stopped, and the interlock is what stops the last.

The other 2 pass already, because they test the parts that are given.

```
rcpp verify 33-08
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/mobile/fetch_carry.hpp`.

```cpp
rc::mobile::Mission mission;
const rc::mobile::MissionCmd cmd = rc::mobile::mission_step(mission, sensed);   // every control tick
```

**What this does not do.** The drive and arm motions are numbers (a station, a place for the arm): the planners (26, 30), the parking choice (33-03) and the whole-body motion (33-04) are what turn them into movement, and a real stop is not instant. There is one part, two stations; retrying a grasp does not re-park the base. The battery check is one number at the start, and a low battery in the middle (33-06's range) is only checked by the time limits. The tipping limits of 33-05 are used by the interlock only through the arm being tucked: a base carrying a heavy part tucked can still be limited in its stopping. Recovery from a fault is for the cell, not the machine.

## What Breaks First

- **A mission with no check after each step.** See `E-MOB-0022`.
- **A mission that is stopped by a fault with no reason.** See `E-MOB-0023`.
- **Driving with the arm out because the plan said so.** See `E-MOB-0024`.

## Ship It

`rc/mobile/fetch_carry.hpp` holds `MState`, `MissionIn`, `MissionCmd`, `Mission`, `guard` and `mission_step`.
