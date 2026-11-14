# Wheels and a Gyro in a Filter: The Wheels Tell the Gyro Its Bias When They Say the Robot Is Going Straight, and the Gyro Tells the Wheels They Slipped

> Each sensor is wrong in a way the other is not, and a filter is what lets each one correct the other where it is right.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-06, 23-07, 23-08, 21-10

## The Problem

A robot has two ways of knowing how much it has turned, and they fail in opposite ways. A **gyro** measures the turn directly and is fast and smooth, but it reads a little even when the robot is still, and that little, its **bias**, wanders with the temperature and adds up: 0.01 rad/s is 34 degrees in a minute (lesson 22-06). The **wheels** have no bias, because they read zero when they do not move, but they say what the wheels did and not what the robot did, so they are wrong by a share of every turn, and wrong by a great deal when a wheel slips on loose ground. Each is the cure for the other's fault. What is needed is a way to let the wheels teach the gyro where the wheels are right, and let the gyro refuse the wheels where they are wrong.

## The Concept

### The bias, measured by the wheels

The gyro reads the turn plus its bias. The wheels give a turn with no bias but wrong by a share of itself. So the **difference** between the two, `gyro - wheels`, is the bias plus the wheels' error. When the robot is going straight the wheels' turn is nothing and their error is nothing, and the difference is the bias alone. `GyroBiasFilter` is a scalar Kalman filter on the bias. Its measurement is that difference, with a variance of the gyro's noise squared **plus the square of the share of the wheels' turn rate**: when the wheels turn at 0.3 rad/s and are wrong by 5 percent, the measurement is uncertain by 0.015 rad/s, three times the gyro's noise, and counts for much less than when the robot is going straight. In the test, after the same difference of 0.05 rad/s for 2 s, the filter that saw it with the wheels straight had moved its estimate of the bias to more than 0.03, and the one that saw it with the wheels turning had moved much less.

### The slip, refused

When a wheel slips the wheels' turn is wrong by half or more, and the difference is not the bias. The filter has a **gate**: a difference further than a few standard deviations from the estimate is refused, and counted. In the test a difference of 0.2 rad/s from a filter that had settled on 0.01 was refused and left the estimate unchanged. The heading is the gyro's less the bias, and the slip never reaches it. See `E-VEH-0055`.

### Over a run

120 s of a robot going straight for 15 s and turning for 10, alternately left and right, with a gyro bias of 0.01 rad/s that wandered, a wheel turn that was 97 percent of what the wheels said and, at 50 s, a slip that halved the turn for 2 s. The heading from the wheels alone ended **3.1 degrees** out (the slip and the 3 percent), the gyro alone **68.0 degrees** out (its bias), and the filter **1.0 degrees**, with the bias estimated as **0.0089 rad/s**, near the 0.01 it was. Driven at 0.5 m/s, the position at the end was **1.8 m**, **14.1 m** and **0.3 m** from the truth. The filter was the best of the three on both. With wheels that have been calibrated, an estimator that gives them the same weight whenever (the "naive" one) does nearly as well (**1.4 degrees**). With wheels that are 12 percent out, a track not calibrated, it ended **3.7 degrees** out on average over five runs, against **1.7** for the one that weighs them by how straight they are: it takes the wheels' error for bias every time the robot turns. See `E-VEH-0056`.

## Build It

`GyroBiasFilter::update` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- The measurement of the bias is gyro - wheel_rate, with a variance of sigma^2 + (share * wheel_rate)^2; if it is within gate standard deviations of the estimate, a scalar Kalman update; otherwise count it as refused.

As shipped, all 4 of the 4 tests fail. They are, in the order the suite runs them:

- With no motion the bias of the gyro is read from the gyro, and with the wheels going straight it is read from the difference.
- When the wheels are turning they are trusted less, and a difference far from the estimate is refused as a slip.
- Over a run of two minutes the filter keeps the heading, where the gyro alone drifts and the wheels alone are caught by a slip.
- Weighing the wheels by how straight they are matters when the wheels are not well calibrated.

```
rcpp verify 25-01
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/locate/gyro_wheels.hpp` opens the module `rc::locate`.

```cpp
rc::locate::FusedHeading heading(gyro_noise, 0.05, 1e-4, 3.0);
heading.update(gyro_z, wheel_turn_rate, dt);
const double theta = heading.theta();
```

**What this does not do.** The heading still drifts, by whatever the bias estimate is wrong by, and a robot that never goes straight never learns its bias: the estimate needs the straight stretches, and a vehicle that turns all the time must learn it from being still (the lesson on stationary updates) or from something that measures heading. The wheels' share of error is a constant here, and on loose ground it is not. The filter has no model of the gyro's scale error. And it estimates the heading's rate error, not the heading: nothing in it knows which way is north.

## What Breaks First

- **A slip believed.** See `E-VEH-0055`.
- **A bias estimated by trusting the wheels equally at every turn rate.** See `E-VEH-0056`.
- **A gyro integrated with no bias at all.** See `E-VEH-0057`.

## Ship It

`rc/locate/gyro_wheels.hpp` holds `GyroBiasFilter` and `FusedHeading`.
