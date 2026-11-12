# When a Measurement Is From the Past: Stamps, a History of the State, Correcting the State as It Was and Carrying the Correction Forward, and Finding the Delay Between Two Clocks

> A fix that arrives now tells you where the robot was a quarter of a second ago, and a quarter of a second at 2 m/s is half a metre.

**Type:** Build
**Time:** about 85 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 25-03, 25-01, 24-07, 21-10

## The Problem

A measurement is never about now. A GNSS fix is computed from signals received some time before it arrives; a lidar scan is stamped when its sweep ended and reaches the computer a little later; a camera frame is taken, processed and handed over. By the time it is used, the robot has moved. At 2 m/s a quarter of a second is half a metre, and a filter that treats the fix as the position **now** is wrong by exactly that, in the direction of travel, every time. It is not noise: it is a bias, and it is the same size as the speed times the delay.

## The Concept

### The stamp

Every measurement carries the time at which it was **taken**, not the time at which it arrived. Where the sensor does not give a stamp, the time of arrival less a measured delay is the best there is, and it is a second source of error. The rest of this lesson assumes the stamp is right.

### A history, and a correction carried forward

`StateHistory` keeps the recent states of the estimator with their times. `at(t)` returns the state at a **past** moment, interpolated between the two states round it: asked for t = 0.45 s of states every 0.1 s on a straight run, it gave the position half way between, and it says no for a time before the first state or after the last.

`delayed_correction` compares the measurement with the state **as it was at the stamp**, and the difference, times a gain, is the correction. The correction is then added to the state **now**. A fix of 1.1 m taken at 0.5 s, when the estimate was 1.0, is 0.1 m out; compared with the state now, 2.0 m, it would have been 0.9 m out in the other direction, which is the whole error of the naive way.

### Three ways, one run

A robot going at 2 m/s with hard accelerations (the speed swings by 1.2 m/s every 3 s), on an odometry that reads 1 percent long, was corrected once a second by a fix good to 3 cm that arrived 0.25 s late:

- **taking the fix for the position now**: a steady bias of **0.49 m**, and an rms error of **0.493 m**;
- **extrapolating the fix** by the velocity over the delay: **0.035 m** rms;
- **comparing it with the state of its own time** and carrying the correction forward: **0.045 m**.

The last two are the same to within the noise of the fix (3 cm), and an order of magnitude better than the first. Extrapolating is the cheaper and is right as long as the velocity is the velocity over the whole delay. When it is not, it is not: a robot going at 2 m/s that **brakes to a stop during the delay**, as in an emergency, has a fix that arrives 0.25 s after it was taken and 0.25 m short of where the robot has ended. Moved on by the velocity it was **0.25 m beyond** the robot; compared with the state of its own time it was **0.000 m** out. The history way needs the estimator to remember, and a long enough memory (here 4000 states) to reach back past the longest delay it will meet. See `E-VEH-0076`.

### The delay between two clocks

Two sensors on a robot often do not share a clock. A gyro and the wheels' turn rate are the same motion, seen twice; if one is late the two signals are the same shape shifted. `estimate_lag` tries shifts and keeps the one with the largest correlation. With the gyro **0.12 s** behind the wheels and noise on both, it found **0.12 s**. A lag of 40 ms between a gyro and a camera is, at a turn of 1 rad/s, **2.3 degrees** of heading in every image. See `E-VEH-0077` and `E-VEH-0078`.

## Build It

`StateHistory::at` in `exercise/solution.hpp`.

```
rcpp verify 25-08
```

## Use It

`rc/locate/time_align.hpp` opens nothing new in `rc::locate`.

```cpp
history.add(Stamped{t, x, y, theta});          // every step
const rc::locate::Correction c = rc::locate::delayed_correction(history, fix.stamp, fix.x, fix.y, 0.5);
if (c.valid) { x += c.dx; y += c.dy; }
```

**What this does not do.** The correction is carried forward unchanged, which is right for a position and wrong for anything that the correction should have changed on the way (the heading, if the fix changed it, would have carried the position along a different path in the interval). The proper way is to go back to the state at the stamp, apply the measurement, and run the odometry forward again over the interval, which costs a stored history of the odometry as well as of the states. And the lag estimate gives one number for a whole run, where a real delay changes with the load on the computer.

## What Breaks First

- **A fix stamped with the time it arrived.** See `E-VEH-0076`.
- **A delay corrected by extrapolation through a manoeuvre.** See `E-VEH-0077`.
- **Two clocks that were never compared.** See `E-VEH-0078`.

## Ship It

`rc/locate/time_align.hpp` holds `Stamped`, `StateHistory`, `Correction`, `delayed_correction` and `estimate_lag`.
