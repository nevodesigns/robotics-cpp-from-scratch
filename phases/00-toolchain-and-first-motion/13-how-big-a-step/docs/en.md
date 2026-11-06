# How Big a Step: The Error in One Step of Motion, and Two Ways to Take It Down

> Lesson 00-05 moved a robot in small steps and said the error shrinks with the step. This lesson measures it,
> and finds a step that needs a hundredth of the work.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 00-05

## The Problem

The robot of lesson 00-05 moves in steps, and for each step it points where it was pointing at the start, goes forward, and turns. That is fine on a straight line. On a circle it
is slightly wrong at every step, always in the same way, and the errors add up.

How wrong is it, and what does it cost to make it right? The only fix shown so far is a smaller step, which costs time. There are better fixes.

## The Concept

### A circle with a known answer

Forward speed 1 m/s and a turn of 1 rad/s is a circle of radius 1 m. A quarter of it takes pi/2 seconds and ends at (1, 1), facing up. The wheel speeds that make it, on a 0.5 m wheel base, are 0.75 and 1.25.

### The step of 00-05

The whole step goes along the heading the robot had at the start. During the step the robot turns, so every step goes a little too far outward. Driving that quarter circle:

| steps | 10 | 20 | 40 | 80 | 160 | 320 |
|---|---|---|---|---|---|---|
| miss, mm | 111.110 | 55.541 | 27.769 | 13.884 | 6.942 | 3.471 |

Twice the steps, half the error. The robot ended further from the centre than the circle's radius at every count, and it took **1111** steps to be within a millimetre. See `E-NUM-0031`.

### The heading halfway through

Use the heading the robot has halfway through the step. The start is too early and the end too late by equal amounts, so halfway is right for both:

```cpp
const double halfway = start.theta + turn * dt / 2.0;
```

| steps | 10 | 20 | 40 | 80 | 160 | 320 |
|---|---|---|---|---|---|---|
| miss, mm | 1.455 | 0.364 | 0.091 | 0.023 | 0.006 | 0.001 |

Twice the steps, **a quarter** of the error, and it needed **13** steps to be within a millimetre. It is the same amount of work per step, one extra addition. See `E-NUM-0032`.

### The exact arc

For constant wheel speeds the path is a circle of radius forward / turn, and its end point has a formula. The miss was under 1e-11 mm at every count, so **one** step is as good as three hundred.

The formula divides by the turn, so a robot going straight has to be handled on its own: below a turn of 1e-9, move forward along the heading. See `E-NUM-0033`.

### Why not always the exact one

It is exact only while the wheel speeds are constant for the whole step. A real robot's speeds change every control cycle, and then the step has to be short enough that they are about constant, and the exact formula
and the midpoint one are about equally good. The midpoint step also extends to cases where there is no circle formula.

## Build It

`step_midpoint` and `step_arc` in `exercise/solution.hpp`. The 00-05 step, `drive_with` and the constants are given.

```
rcpp verify 00-13
```

## Use It

`rc/sim/arc.hpp` joins `rc::sim`.

```cpp
pose = rc::sim::step_midpoint(pose, left, right, wheel_base, dt);
```

**What this does not do.** It assumes the wheel speeds do not change during the step, and it models a robot that does not slip. It has no noise. The error in a real robot's odometry is mostly the wheel size and the wheel base
being wrong, which a better step cannot fix (lesson 16-09). And the heading is wrapped by the 00-05 step and is not by these two, so a caller that compares headings should wrap them.

## What Breaks First

- **The step of 00-05 on a curve.** It runs wide, and halving the step only halves the miss. See `E-NUM-0031`.
- **The wrong end of the step.** It runs short by the same amount. See `E-NUM-0032`.
- **The exact formula for a straight line.** It divides by zero. See `E-NUM-0033`.

## Ship It

`rc/sim/arc.hpp` holds `step_euler`, `step_midpoint`, `step_arc` and `drive_with`.

Lesson 06-06 returns to this with energy as the measure. The question is the same: how big a step.
