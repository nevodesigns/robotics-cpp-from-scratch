# A Robot That Knows How Lost It Is: Dead Reckoning, Beacon Fixes, and a Fence Buffer That Is Only as Good as the Drift It Assumes

> A robot that does not know where it is can still be safe, if it knows how much it does not know.

**Type:** Capstone
**Time:** about 140 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 21-04, 19-14, 19-06, 16-02, 16-01, 19-10

## The Problem

The fifth capstone is about **position**. Every robot so far has either not needed one (the line follower, the balancer) or been given a perfect one (the rover, the arm). A real mobile robot has to work its position out from its wheels, which are not quite the size the software thinks, and from the occasional help of a landmark. The fence of lesson 19-06 is only as good as that position, and its buffer was a number someone chose. This lesson finds out what the number should be.

The pieces: the odometry of lesson 16-01, the filter of 16-02, the fence and its buffer from 19-06, and the supervisor.

## The Concept

### What the wheels get wrong

The real left and right wheels travel `left_scale` and `right_scale` times the distance asked of them, and the real wheel base is `base_scale` times the one in the software. Driving **straight on**, with equal distances asked of both wheels, the position error after 10, 20 and 30 m:

| fault | 10 m | 20 m | 30 m |
|---|---|---|---|
| wheel base 5 percent out | 0.00 | 0.00 | 0.00 |
| left wheel 1 percent big | **1.66** | **6.55** | **14.49** |

A wheel base that is wrong costs **nothing** going straight, since it only ever divides the *difference* between the wheels (the finding of 16-01, now across a lesson). A wheel that is 1 percent big turns the robot a little all the time: the heading error grows in proportion to time, and the position error in proportion to its **square**. 4.0 times the error at twice the distance, and 8.7 times at three times: about 4 and 9, as a square says. A robot that is in error by a centimetre a metre at the start of a run is in error by a **metre** at 100 m. See `E-CAP-0013`.

And the robot **does not know**. Its belief goes straight along the axis: the odometry has no way to see that the truth has turned. It is in the middle of the corridor, by its own account, while it leaves it.

### Fixes

A beacon (a marker the robot can see, a radio ranging, a GPS) gives a position with some error. `apply_fix` takes it into a filter on each axis (16-02) and sets the believed position. Over 30 m with the 1 percent wheel, and fixes with a standard deviation of 0.1 m:

| fixes | number | error at 30 m | largest error |
|---|---|---|---|
| none | 0 | 14.49 m | 19.66 m |
| every 10 s | 6 | 5.04 | 6.11 |
| every 5 s | 13 | 2.86 | 3.51 |
| every 2 s | 34 | **1.42** | 1.86 |

The error is **bounded by the time between fixes**, and not by the quality of any one of them: the fixes are good to 0.1 m and the best the robot can do is 1.4 m, because of how far it drifts between them (and the fix does not correct the **heading**, which is what is wrong, so the drift starts again). Fixes every 5 s that **die at 20 s**: 3 fixes, and an error of **13.75 m** at 30 m, close to none at all. A robot with a position source should know when it has stopped getting it. See `E-CAP-0013`.

### A fence on a belief

The corridor is 3 m wide. The true position of the robot with the 1 percent wheel crosses its edge after **9.6 m**, and it goes on to a lateral position of **-18.3 m**, **believing itself on the axis**. A fence on the believed position, with no buffer, **never trips**: the belief never leaves the corridor. A fence with the right polygon and the wrong position is not a fence. See `E-CAP-0014`.

### The buffer, and what to assume

Lesson 19-06's answer was a buffer that grows with the distance since the last fix: the stopping distance plus an assumed drift for each metre. With a stopping distance of 0.5 m:

| assumed drift | stops after | the truth was then |
|---|---|---|
| 0.05 m per metre | 20.1 m | **OUTSIDE the corridor** |
| 0.10 | 10.1 m | **OUTSIDE** |
| 0.20 | 5.1 m | inside |
| 0.40 | 2.6 m | inside |

The first two stopped the robot **after** the truth had left: the fence "worked" and the robot was outside. What is the right number? The drift when the truth reached the edge was 1.5 m over 9.6 m, **0.156 m for each metre**, and it is **still growing**. A constant is the wrong shape: a drift that goes with the square of the distance is a drift **per metre** that rises with the distance (here it is about 0.017 times the distance, so 0.083 at 5 m and 0.167 at 10 m). The number to assume is the drift for the **longest distance the robot will go without a fix**. That is a decision about the route and the beacons, and not about the odometry. See `E-CAP-0014`.

### Fixes and the buffer together

The same robot with a fix every 5 s:

| assumed drift | stops after | the truth was then |
|---|---|---|
| 0.05 | 10.1 m | OUTSIDE |
| 0.10 | 9.9 m | OUTSIDE |
| 0.20 | **8.7 m** | inside (lateral -1.28) |

With fixes the robot has a much smaller distance since the last fix to cover, and it goes **8.7 m** before it must stop where without them it stopped at **5.1 m**, with the same assumed drift, the same safe outcome, and 3.6 m more done. The beacon does not change the safety argument; it buys **range**. See `E-CAP-0015`.

## Build It

`position_error`, `stop_buffer` and `apply_fix` in `exercise/solution.hpp`. The truth and belief, the beacon, the fence wiring and the run are given.

```
rcpp verify 21-05
```

## Use It

`rc/capstone/localisation.hpp` joins `rc::capstone`.

```cpp
LocalisationConfig config;
config.beacon_period = 5.0;
config.use_buffer = true;
config.assumed_drift = 0.2;          // the drift for the longest distance between fixes, not the drift at the start
const LocalisationResult r = run_localisation(config, 70.0);
```

## Show It

The table to show is the one with the **assumed drift in the first column and "OUTSIDE" in the last**. It shows that you know a safety margin is a number somebody has to get right, and that you found out what right is. Next to it put the square law (4 and 8.7), since it is what makes "the robot is accurate to 1 percent" mean nothing without a distance.

**What this does not do.** It drives in a **straight line** with an error in one wheel and a position fix, in one dimension of danger (the corridor's width). A real robot turns, which changes which errors matter (a wheel base wrong by 5 percent costs a great deal in a turn, which lesson 16-01 measured), has a heading source (a gyro, a compass) that this lacks, and fuses fixes with a proper filter with a covariance matrix and not two scalar ones. The beacon's noise is Gaussian and independent, and a real one has outliers, multipath and dropouts that look like good fixes. The fix sets the position and **not the heading**, which is why the error returns so fast. The drift rate is for this robot's wheels. And a fence that stops the robot at the right moment is a stop **inside** the corridor, which a corridor with a drop at its edge needs; a corridor with a person at its edge needs more.

## What Breaks First

- **A position error quoted without a distance.** See `E-CAP-0013`.
- **A fence on a belief that cannot see the fault.** See `E-CAP-0014`.
- **An assumed drift per metre taken from the start of a run.** See `E-CAP-0014`.
- **A beacon that is trusted after it has died.** See `E-CAP-0015`.

## Ship It

`rc/capstone/localisation.hpp` holds `WheelError`, `position_error`, `stop_buffer`, `apply_fix`, `LocalisationNoise`, `LocalisationConfig`, `LocalisationResult` and `run_localisation`.
