# Changing Gait Without Stumbling: Phase Tables for Walk, Trot, Pace and Bound, Turning One Into Another at a Bounded Rate, and a Switch With a Gap

> A gait is a table of offsets; changing gait is moving the offsets, and they may not move faster than the feet can follow.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 35-02, 34-04, 26-07

## The Problem

A quadruped uses **different gaits at different speeds**: a **walk** (slow, stable, 35-01), a **trot** (faster, 35-02), sometimes a **pace** (the legs of one side together) or a **bound** (the front pair, then the rear pair). A gait is **a table**: the **phase offset** of each leg (34-04). To **change** gait the table has to change, **while the robot is moving**. Changed carelessly, **a leg that was in mid-stance is suddenly told it is in swing** and its foot is commanded to **jump**.

## The Concept

### Four tables

With the legs in the order front left, front right, rear left, rear right: a **walk** has offsets 0, 0.5, 0.25, 0.75 (one foot at a time, left side then right); a **trot** 0, 0.5, 0.5, 0 (the **diagonals** together); a **pace** 0, 0.5, 0, 0.5 (the **same side** together); a **bound** 0, 0, 0.5, 0.5 (the **front pair** then the **rear pair**). The test checks the pairings and that a walk of duty 0.75 always has three feet down. See `E-LEG-0030`.

### Moving the offsets, by the short way round

To change gait, **move each leg's offset toward the new gait's**, at **no more than a rate** (in cycles per second). A phase is a **point on a circle**: **0.95 and 0.05 are 0.1 apart, not 0.9**. So the difference is **wrapped to the range minus a half to a half** before it is limited, and the offset is wrapped back to 0 to 1. Starting at 0.95 with a target of 0.05 the offset went **forward through the wrap**, not back by 0.9 (the test). A **tie** (exactly half a cycle) goes one way or the other, and a real gait avoids it. See `E-LEG-0031`.

### What goes wrong if it is abrupt

A walk changed into a trot at the **worst moment** kept at least **2** feet on the ground when the offsets were **switched at once**, and **2** at a rate of **0.2 cycles per second**: **the number of feet does not show the danger**. What does is the **jump of a leg's phase**: the abrupt change moved one by **0.250** of a cycle **in one tick**, which moves the **commanded foot** along its path by that much (a foot in stance suddenly told to be in swing), where the bounded rate moved it **0.0002** a tick, taking **1.25 s** in all. A leg cannot follow a jump: it **lags**, **drags** or **hits** (34-05). See `E-LEG-0032`.

### Choosing the gait from the speed

The gait is chosen from the **speed**, and with **a gap**: change up to the trot **above** one speed and back to the walk only **below a lower** one. With a speed **hovering around 0.5 m/s** (between **0.3** and **0.7**) a single switch speed **flapped 175 times in 400 readings** and the gap **0**. Every flap is a gait change, with the cost above.

## Build It

`step_offsets_toward` in `exercise/solution.hpp`.

```
rcpp verify 35-03
```

## Use It

`rc/legged/gait_schedule.hpp`.

```cpp
gait = rc::legged::gait_for_speed(speed, gait, 0.6, 0.4);
rc::legged::gait_offsets(gait, target);  rc::legged::step_offsets_toward(offsets, target, 0.2, dt);
```

**What this does not do.** The **duty and the period** are held fixed during a change (they would be moved at a bounded rate too, 34-04). The change is **linear per leg** and **does not check** that the **stability** (35-01, 35-02) holds in between: only the **count of feet** is checked. The speed is **given** and **noise-free**. Gaits **other than the four** (a gallop, a pronk) are not covered. The rate of 0.2 cycles per second is **a number** chosen here, and a real one comes from what the legs can follow.

## What Breaks First

- **A phase that was not wrapped.** See `E-LEG-0031`.
- **An abrupt switch of the table.** See `E-LEG-0032`.
- **One speed for the switch both ways.** See `E-LEG-0030`.

## Ship It

`rc/legged/gait_schedule.hpp` holds `GaitKind`, `gait_offsets`, `step_offsets_toward`, `gait_for_speed` and `min_feet_down`.
