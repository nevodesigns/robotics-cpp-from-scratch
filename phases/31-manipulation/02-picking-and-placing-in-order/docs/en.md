# Picking and Placing in Order: A State Machine With a Check After Every Step That Can Fail

> A pick is a short list of steps, and the part that makes it work is what the machine does when a step did not do what it was meant to.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 31-01, 30-08, 26-10

## The Problem

31-01 said how hard to squeeze. A pick is not one action but a list: go above the part, go down, close, lift, carry, lower, open, go back up. Each step can fail: the hand closes on nothing, the part falls on the way, the arm never gets there. A script that does the steps in order and never looks has a robot that carries on with an empty hand. The structure that handles this is a state machine with a check after each step that can fail.

## The Concept

### The steps

The machine goes: approach (above the part), descend, close, verify, lift, transit (above the place), lower, release, retreat, done. It is given what was sensed (the target the arm has reached, the gap between the fingers, whether the hand has stopped) and returns what to do next (a target and open or closed). It holds no robot; it is a function from what was sensed to what to do, which is why it can be tested with a small world. A run that went to plan took 36 ticks of 0.1 s. See `E-ARM-0079`.

### The check that decides

After closing, the gap says whether the grasp worked. A hand closed on the part has the part's width, within a tolerance; a hand closed on nothing has a gap near zero; a hand closed on the wrong thing has another width. `holding` is that comparison. A hand that closed on nothing and was given a fresh try took 48 ticks with 1 retry for 1 miss. See `E-ARM-0080`.

### Retrying, and giving up

A missed grasp goes back to approach (the hand opens and rises, then comes down again). It is allowed a limited number of tries: a hand that missed every time stopped in fault 1 (no part) after 3 tries, and did not go round for ever.

### What goes wrong on the way

While the part is carried (lift, transit, lower) the gap is watched at every tick: a part that fell was seen at once and stopped the machine in fault 2. Every state also has a time limit: a move that never arrived stopped the machine in fault 3 at tick 62. See `E-ARM-0081`.

## Build It

`holding` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- True if the gap is within tol of the part's width (a closed hand on nothing has a gap near 0).

As shipped, 3 of the 4 tests fail, and these are the ones your code has to turn green:

- The grasp check is true for the part's width and false for a closed hand or a wrong part.
- A missed grasp backs off and tries again, and a hand that always misses stops with the reason.
- A part that falls on the way is seen from the gap, and a move that never arrives times out.

The other one passes already, because it tests the parts that are given.

```
rcpp verify 31-02
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/arm/pick_place.hpp`.

```cpp
rc::arm::PickMachine m;
rc::arm::PickCmd c = rc::arm::pick_step(m, sensed);   // every control tick
```

**What this does not do.** The arm's motion between targets is not here: a target is a number that a planner (phase 30) turns into a path. The gap is taken as the only sign of a grasp; a force or a camera would be better and come in the next lessons. The stop in fault leaves the hand as it was and nothing recovers from it, which is a decision for the cell. There is one part, one place.

## What Breaks First

- **Doing the steps with no check.** See `E-ARM-0079`.
- **A grasp taken as made because the hand closed.** See `E-ARM-0080`.
- **A wait with no time limit.** See `E-ARM-0081`.

## Ship It

`rc/arm/pick_place.hpp` holds `PickMachine`, `PickIn`, `PickCmd`, `holding` and `pick_step`.
