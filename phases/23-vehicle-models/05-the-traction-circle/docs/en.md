# The Traction Circle: Turning, Speeding Up and Braking Share One Budget of Grip, and a Vehicle That Spends It All on One Has Nothing Left for the Others

> Grip is a budget, not three separate allowances, and the planner that forgets it asks for a turn and a brake at once.

**Type:** Build
**Time:** about 80 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 23-03, 22-05, 21-10

## The Problem

Lesson 22-05 gave each surface a friction and lesson 23-03 gave the car a fastest speed round a circle. Both treat the grip as something to be spent on one thing at a time. A tyre has one budget. If a turn is taking most of it, then there is little left to brake with or to speed up with, and a planner that asks for the full braking of the straight line in the middle of a bend is asking for a slide. This is the **friction circle**, and it is the single most useful picture of what a vehicle can do on a given surface.

## The Concept

### One budget

On a slope of angle `alpha` the grip is `mu * g * cos(alpha)`. The sideways acceleration of a turn at speed `v` and curvature `kappa` is `v * v * kappa`; the longitudinal acceleration along the line of travel is the speeding up or the braking. The tyres can give any pair of them whose squares sum to the square of the grip: `a_lat^2 + a_long^2 <= grip^2`. A circle.

`traction_budget` takes a terrain cell (the surface and slope of lesson 22-05), a heading, a speed and a curvature, and returns the sideways acceleration the turn needs, the **share** of the grip it takes, and what is left for the line of travel: `grip * sqrt(1 - share^2)`, with the pull of gravity along the slope and the rolling resistance taken off for speeding up and added for braking.

### On gravel, at the smallest circle of a car

At 1.5 m/s round the smallest circle of the car of lesson 23-03 (radius 0.742 m) on gravel, the turn took **69 percent** of the grip. On a straight line the vehicle could speed up at **4.02 m/s2** and brake at **4.81**; in the turn, **2.82 and 3.60**. Stopping from 1.5 m/s took **0.23 m** on the straight and **0.31 m** in the bend. The numbers are checked against the circle itself: the squares of the two accelerations in use add up to the square of the grip, to 1e-9. See `E-VEH-0013`.

### The edge

The turn alone uses all of the grip at `sqrt(mu g / kappa)`: on this circle, **1.81 m/s**. A hair below, the vehicle holds; a hair above (`slides`), it does not, and its `drive` is negative: it cannot even hold its speed. Ice reaches the edge at a lower speed on the same circle. `max_speed_for_curvature` and `max_curvature_at_speed` are the same limit solved for the speed and for the curve. A speed limit that is a number for the vehicle and not a function of the bend is wrong somewhere. See `E-VEH-0014`.

### Near the edge there is nothing left

At 99 percent of the sliding speed the turn takes 98 percent of the grip, and what is left for braking is the square root of the remainder: **20 percent** of the braking of the straight line. A vehicle in a fast bend that is asked to brake hard does not brake: it slides. The planner of a later phase uses this budget to slow before a bend and not in it. See `E-VEH-0015`.

### A slope

A slope takes its share first. Heading up a slope of 0.2 on gravel the vehicle has less to speed up with and more to brake with (gravity is helping), and the difference between them is twice the slope force plus the rolling resistance, as it must be.

## Build It

`traction_budget` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- Share is lateral over grip; if it is 1 or more the tyres have let go; otherwise the grip left for the line of travel is grip * sqrt(1 - share^2); drive is that less gravity along the slope and the rolling resistance, brake is that plus both.

As shipped, all 4 of the 4 tests fail. They are, in the order the suite runs them:

- On a straight line the budget is the whole grip, less the rolling resistance.
- A turn takes a share of the grip and the rest is what is left to speed up and to brake with.
- Past the edge the tyres let go, and the speed at which that happens is the one the limit gives.
- Near the edge there is almost nothing left to brake with, and a slope takes its share first.

```
rcpp verify 23-05
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/vehicle/traction.hpp` opens nothing new in `rc::vehicle`.

```cpp
const rc::vehicle::TractionBudget b = rc::vehicle::traction_budget(terrain.at(x, y), heading, speed, curvature);
if (b.slides || b.brake < needed_decel) slow_down();
```

**What this does not do.** It is a circle, where a real tyre's limit is an ellipse, with more grip along the wheel than across it, and one that shrinks with slip and with speed. All four tyres share the load equally, and nothing moves the weight between them as the vehicle brakes or turns, which is the largest error of the model at the edge. There is no tyre model, no suspension and no aerodynamics. It is a limit on what a planner may ask, and an upper one.

## What Breaks First

- **A braking limit taken from the straight line.** See `E-VEH-0013`.
- **A speed limit that does not depend on the bend.** See `E-VEH-0014`.
- **Braking in the bend.** See `E-VEH-0015`.

## Ship It

`rc/vehicle/traction.hpp` holds `TractionBudget`, `traction_budget`, `max_speed_for_curvature` and `max_curvature_at_speed`.
