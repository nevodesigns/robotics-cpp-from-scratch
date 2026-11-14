# How Lost Can It Be: The Bound on Dead Reckoning Between Fixes, Why It Grows Faster Than the Distance, and When the Robot Must Stop and Ask

> An estimate without an error bar is a hope, and the error bar of dead reckoning is a number you can work out before you set off.

**Type:** Build
**Time:** about 85 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 25-01, 25-03, 23-07, 21-10

## The Problem

Between fixes a robot lives on its odometry, and the odometry is wrong in ways that add up (lesson 23-07). A robot that carries on regardless is one that goes where it believes it is, and a robot that carries a **number for how wrong it can be** can do something about it: slow down, stop, go back to where there is a fix, or refuse a task that needs an accuracy it does not have. That number can be worked out before the robot sets off, from the odometry's own noise, by carrying the uncertainty along with the pose.

## The Concept

### Carrying the covariance

`DeadReckoning` holds the pose and its 3 by 3 covariance. Each odometry step is a distance and a turn; the distance is wrong by a share of itself (the scale error, 1 percent here), the turn by a share of itself (3 percent), and the heading **wanders as the robot goes**, by 0.005 rad per root metre (a third of a degree after a metre, and 3 degrees after 100). The covariance is carried by the usual rule of an extended filter, `P' = F P F^T + G Q G^T`, where `F` is how the new pose depends on the old (a heading error turns into a position error) and `G` how it depends on the odometry. `radius95` is the radius of the circle that holds the position with 95 percent probability.

### It grows faster than the distance

If only the distance were noisy (an independent 1 percent in each step), the errors would add like a random walk, as the square root of the distance: 10 times the distance, 3.16 times the bound, which the test checks. (A scale error that is the same in every step, a wheel of the wrong size, is in proportion to the distance, and is the one the calibration of lesson 23-08 takes out.) But a heading error does more: a robot that is a degree off its heading and goes a metre is 1.7 cm off, and one that goes 100 m is 1.7 m off, and the heading error itself has been growing all the way. The bound is **0.22 m after 10 m** and **7.07 m after 100 m**: **31.6 times** as much for ten times the distance, where a straight proportion would be 10. A random walk in heading gives an error that grows as the distance to the power of three halves, which is 31.6 for ten times the distance. Doubling the distance a robot goes between fixes more than doubles how lost it can be. See `E-VEH-0070`.

### The bound is checked

Four hundred runs of odometry with that noise, along 100 m, had 95 percent of the robots within **5.10 m** of where the straight line says they should be, where the bound said **7.07 m**. The two agree to about a third, and the bound is a little loose because a circle holds the long axis of an ellipse, which is mostly the error across the path. Turning makes it worse: a robot that zig-zags has a larger bound than one that goes straight over the same distance, because the turn error adds to the heading. See `E-VEH-0071`.

### What it is for

A fix brings the bound down at once: a fix of 5 cm took it to **0.122 m**, and it grows again from there. The same arithmetic, done before the robot sets off, gives the distance it can go before it might be 1 m out, **27 m**, and before it might be 5 m out, **80 m**. That is the spacing of the fixes that a job needs. A task that wants the robot within a metre of a place and a route that has no fix for 80 m do not go together, and `needs_fix` says so. See `E-VEH-0072`.

## Build It

`DeadReckoning::step`, the covariance update, in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- P' = F P F^T + G diag(var_ds, var_dth) G^T, with F = [[1,0,-ds sin],[0,1,ds cos],[0,0,1]] and G = [[cos, -ds sin / 2],[sin, ds cos / 2],[0,1]] at the middle heading.

As shipped, all 5 of the 5 tests fail. They are, in the order the suite runs them:

- With no noise there is no bound, and with the noise of the distance alone the bound grows as the square root of the distance.
- With a heading that wanders the bound grows faster than the distance, as its three halves power.
- A Monte Carlo of noisy runs agrees with the bound.
- Turning adds to the bound, and a fix takes it back and it grows again.
- The distance a robot may go before it might be 1 m out, or 5 m, is worked out before it sets off.

```
rcpp verify 25-06
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/locate/dead_reckon.hpp` opens nothing new in `rc::locate`.

```cpp
rc::locate::DeadReckoning dr((rc::locate::OdometryNoise()));
dr.reset_to(x_fix, y_fix, heading, 0.05, 0.02);
dr.step(ds, dtheta);
if (dr.needs_fix(1.0)) go_where_there_is_a_fix();
```

**What this does not do.** The numbers of the noise are those of a typical small robot and must be measured for a real one (lesson 23-08 is where to start). The noise is taken to be Gaussian and independent from step to step, which wheel slip on a particular floor is not: a robot that slips on one side of a field slips on it every time. The bound is for the position only; it says nothing about the heading it is used with except through the position. And the Monte Carlo is of the same model: it shows that the arithmetic is right and not that the model is.

## What Breaks First

- **Distance between fixes chosen by feel.** See `E-VEH-0070`.
- **An estimate with no error bar.** See `E-VEH-0071`.
- **A task asked of a position that cannot support it.** See `E-VEH-0072`.

## Ship It

`rc/locate/dead_reckon.hpp` holds `OdometryNoise` and `DeadReckoning`.
