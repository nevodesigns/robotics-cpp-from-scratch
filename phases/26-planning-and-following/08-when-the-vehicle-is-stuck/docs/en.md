# When the Vehicle Is Stuck: Noticing It Without Crying Wolf, a Ladder of Things to Try Per Vehicle, and Knowing When to Give Up

> A vehicle that is stuck and does not know it is just a very expensive paperweight with its motors on.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 26-04, 26-06

## The Problem

Every robot in the field gets stuck. A wheel drops into a rut, a branch lifts it, someone stands in front of it, the map was wrong about a gap. The follower of 26-04 does not know: it keeps commanding forward, the wheels spin or the motors stall, and the vehicle sits there, using its battery, until someone comes. A field robot has to notice, try something, and **give up in an orderly way** when nothing works.

## The Concept

### Noticing, without crying wolf

The detector asks one question: **for a whole window (3 s), was the vehicle commanded to move, and did it move less than 0.1 m?** Both halves matter. A vehicle commanded to stop (at a stop line, 26-07) is not stuck: commanded to stop for 10 s it was flagged **0** times. A vehicle commanded to move and not moving is. The threshold is a trap in the other direction: a vehicle crawling at 0.02 m/s (in a turn, on a floor speed, 26-06) was flagged stuck after **3.0 s** because 6 cm in 3 s is under 0.1 m. The distance must be under the slowest crawl the vehicle is meant to do. See `E-VEH-0103`.

### A ladder, cheapest first

What to try is a **ladder**, in the order of cost and risk:

1. **Wait.** Whatever is in the way (a person, a gate, a puddle of a second's depth) may go.
2. **Back up** a short way: the vehicle that is snagged on a branch or against a lip can be freed by going the way it came.
3. **Spin** in place to look round (and free a wheel): a differential drive or a skid steer can; a **car cannot**, so its ladder backs up twice instead.
4. **Give up**: stop, report, wait for a person.

A vehicle that cannot reverse or spin gives up after waiting. See `E-VEH-0104`.

A vehicle snagged at 3 m of a 6 m path, which let go once it had backed up to 2.6 m, **finished in 23.5 s** after **2** attempts (the wait did not help; the back up did). Without recovery it had not finished after **120 s**. A wall that never moved was tried **3 times** and given up on after **31.2 s**: bounded, not for ever. See `E-VEH-0105`.

### A recovery must not hang

Each behaviour runs until it is done **or until a time limit**: a back up that is itself blocked would otherwise reverse into the wall for ever. And the attempts are only **forgotten after real progress** (1 m from where the last recovery ended), so a vehicle that backs up, goes forward, and is stuck again in the same place does not start the ladder over each time.

## Build It

`behaviour_for_attempt`, the ladder, in `exercise/solution.hpp`.

```
rcpp verify 26-08
```

## Use It

`rc/drive/unstick.hpp`.

```cpp
rc::drive::UnstickConfig cfg;  cfg.can_spin = false;  // a car
rc::drive::Unstick unstick(cfg);
const rc::drive::DriveCmd cmd = unstick.step(now, pose, follower_command);
if (unstick.gave_up()) report_stuck();
```

**What this does not do.** Back up and spin are blind: the vehicle does not check behind it (it needs a rear sensor, and the lesson has none; a car that reverses into something has made things worse). Nothing re-plans after a recovery, so a vehicle that is stuck because the map is wrong will be stuck again until the map changes (26-09 is about when to re-plan). The ladder is the same on every surface. The detector needs the commanded speed: a vehicle that is commanded to move by a person on a remote (27-02) is not seen here. The distance and the window are fixed numbers, and a vehicle that moves slowly on purpose needs its own.

## What Breaks First

- **A detector that cries wolf, or sleeps.** See `E-VEH-0103`.
- **A car told to spin.** See `E-VEH-0104`.
- **A recovery that never ends.** See `E-VEH-0105`.

## Ship It

`rc/drive/unstick.hpp` holds `DriveCmd`, `Behaviour`, `UnstickConfig`, `StuckDetector`, `behaviour_for_attempt` and `Unstick`.
