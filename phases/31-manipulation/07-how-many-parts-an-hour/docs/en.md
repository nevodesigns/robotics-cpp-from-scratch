# How Many Parts an Hour: Cycle Time Built From Its Pieces, the Speed Override It Takes to Hit a Target, and What a Person Standing Near Costs

> A cycle is a sum, and the speed limits of the last lessons are all terms in it.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 31-06, 31-04, 31-01, 30-06, 21-03

## The Problem

A cell is bought for **parts an hour**. 21-03 counted the cycle with a fixed profile. Every lesson since has added a **speed limit** or a **time**: the hand takes time to close (31-01), the way down onto the part has to be slow enough to feel contact (31-04), and a person standing near caps the speed (31-06). This lesson adds them up, so the **cost** of each can be read off, and the **override** needed to meet a target found.

## The Concept

### A cycle is a sum

One cycle is six moves (down to the pick, lift, carry, down to the place, lift, return) and four waits (the hand closing and settling, opening and settling). Each move is a **trapezoid** of distance, speed and acceleration (30-06), at the rated speed times an **override k**, with the acceleration scaled by k squared (the same path in a time stretched by k). The two moves **down onto a contact** are held to the contact speed. At the rated speed a cycle took **3.68 s**, which is **977** an hour. See `E-ARM-0094`.

### What does not stretch

At half the speed the cycle took **5.70 s** and at a quarter **10.13 s**: **less than double and quadruple**, because the hand's **0.8 s** and the contact approaches (**1.07 s**, at a speed that does not follow the override) do not scale. Slowing the robot by half does not halve the throughput, and **speeding it up** does less than it seems: most of a cycle can be the parts of it that do not depend on the robot's speed. See `E-ARM-0095`.

### The override for a target

For a target of **5.16 s** the override needed was **0.576**. It is found by **bisection** because the cycle time falls as k rises but is not a simple function of it (the contact speed does not scale). A target of **2.95 s**, faster than the rated speed can do, gave **-1** (minus one, refused): no override reaches it. The cell must be changed (a faster arm, shorter moves, a faster hand), not told to hurry.

### A person near

The speed a person's distance allows (31-06) caps the tool speed. A person **1.0 m** away cut the speed to **1.23 m/s** and the cycle to **3.77 s**; one **2.0 m** away allowed **2.49 m/s** and a cycle of **3.68 s**. Closer than the distance for standing still the robot stops (minus one). A cell that expects people near has to count this in its throughput. See `E-ARM-0096`.

## Build It

`scale_for_cycle` in `exercise/solution.hpp`.

```
rcpp verify 31-07
```

## Use It

`rc/arm/cycle_time.hpp`.

```cpp
rc::arm::CycleParams cell;
const double k = rc::arm::scale_for_cycle(cell, 3.0);
if (k < 0.0) change_the_cell();
```

**What this does not do.** Every move is a **trapezoid in tool distance**: a real arm's time is set by the **joint** limits along the path (30-06), which are usually worse for some moves, and the path is a straight line. The pick and place is **one part, one place**, with no waiting for a part to arrive, for a camera (31-03) or for a retry (31-02), all of which take time. The hand and contact times are **typical** (31-01). A **speed limit from the person** is applied to the whole cycle although only the moves toward the person need it.

## What Breaks First

- **A throughput quoted at the rated speed.** See `E-ARM-0094`.
- **Speeding up the part that is not the bottleneck.** See `E-ARM-0095`.
- **A throughput that ignores the person.** See `E-ARM-0096`.

## Ship It

`rc/arm/cycle_time.hpp` holds `CycleParams`, `cycle_time`, `throughput`, `scale_for_cycle` and `cycle_with_person`.
