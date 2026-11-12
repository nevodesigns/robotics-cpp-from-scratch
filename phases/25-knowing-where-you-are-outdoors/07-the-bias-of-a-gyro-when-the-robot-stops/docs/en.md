# The Bias of a Gyro When the Robot Stops: Knowing That It Is Still, Reading the Bias From a Few Seconds of Stillness, and How Often a Robot Must Stop to Keep Its Heading

> A robot that is standing still knows exactly what its gyro should read, and every stop is a chance to find out how wrong it is.

**Type:** Build
**Time:** about 80 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 25-01, 25-06, 22-06, 21-10

## The Problem

A gyro's bias wanders, with its temperature and its age, and the robot has to track it. Lesson 25-01 read it from the wheels when they said the robot was going straight. There is a surer way, when the robot gives the chance: **when it is standing still, the gyro should read nothing**, whatever the wheels say, and what it does read is the bias, plus the noise of the reading. A stop of a couple of seconds, every minute or so, is worth more than a minute of straight driving. This lesson is about using it, and about not being fooled about when the robot is still.

## The Concept

### Being sure that it is still

`StillnessDetector` says the robot is still when, **over a whole window** of samples, both wheels have been under a small speed and the accelerometer's magnitude has not varied by more than a limit. A single quiet sample proves nothing, so the window: 25 samples (half a second at 50 Hz) of quiet were needed, and after the wheels turned for one sample the detector needed a whole window of quiet again.

The wheels are not enough. A robot that is picked up and carried, or dragged with its wheels locked, has stopped wheels and is not still: its gyro is turning. A detector that looked at the wheels alone said "still" for **176 of 200 samples** of a robot swinging in someone's hands, and one that also wanted the accelerometer quiet said it for **0**. A bias read from a robot being carried is a disaster, the wrong bias with a small variance. See `E-VEH-0073`.

### Reading the bias

`StillBiasFilter` is the scalar filter of lesson 25-01 with a different source: the bias is a random walk, its variance grows all the time by the walk, and when the robot is still a reading of the gyro is a measurement of it with the gyro's noise. The more samples, the better: the bias read from a stop of **1 s** was good to **0.00063 rad/s** and from a stop of **10 s** to **0.00020**, the noise over the square root of the number of samples (0.005 over √50 is 0.0007). A stop of a couple of seconds is enough: what matters is that it recurs.

### How often to stop

300 s of a robot that turns and drives, with a gyro bias that walks at 3e-4 rad/s per root second: the heading from a bias **read once at the start** ended **9.9 degrees** out (a mean of five runs); with a stop of 2 s every 60 s, **4.6 degrees**; every 10 s, **0.8**. The heading error that an uncertain bias makes grows with time, `bias_sigma * t`, and every stop resets the clock. How often a robot must stop to keep its heading is a number that comes from the walk of the bias and from what the task can bear. See `E-VEH-0074`.

### A step in the bias

A gyro that warms up has a bias that steps. A step of 0.01 rad/s at 100 s was found again **19 s** later, at the next stop. A filter with no stops would not have found it at all. See `E-VEH-0075`.

## Build It

`StillnessDetector::update` in `exercise/solution.hpp`.

```
rcpp verify 25-07
```

## Use It

`rc/locate/stillness.hpp` opens nothing new in `rc::locate`.

```cpp
rc::locate::StillnessDetector still(25, 0.002, 0.2);
rc::locate::StillBiasFilter bias(0.005, 3e-4, 0.0, 0.05);
bias.update(gyro_z, still.update(left, right, accel_norm), dt);
```

**What this does not do.** It needs the robot to stop, and a robot whose job does not let it will need another source (the wheels going straight, GNSS heading). The window and the limits are numbers to fit to the vibration of the robot: one with a running engine is never quiet to 0.2 m/s2. It reads the bias of one axis. And it trusts a gyro that has the same bias when still as when moving, which the temperature of a gyro under load does not always grant.

## What Breaks First

- **A robot taken for still because its wheels have stopped.** See `E-VEH-0073`.
- **A bias read once, at power-up.** See `E-VEH-0074`.
- **A bias that steps and is never looked at.** See `E-VEH-0075`.

## Ship It

`rc/locate/stillness.hpp` holds `StillnessDetector`, `StillBiasFilter` and `heading_error_growth`.
