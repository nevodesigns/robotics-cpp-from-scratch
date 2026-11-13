# An Attitude It Can Trust: A Gyro for the Fast Part, the Accelerometer and the Feet for the Slow Part, and Which of Them Is Wrong When

> The gyro knows how the body is turning and not where it is pointing; the accelerometer and the feet know where, and each is wrong in its own way.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 35-03, 34-03, 25-07, 19-09

## The Problem

Every balance decision (35-05 onward) starts from **how the body is tilted**. 34-03 got it from the **feet**; this lesson adds the sensor every robot has, the **IMU**: a **gyro** (how fast the body is turning) and an **accelerometer** (which way gravity seems to point). Each is **right about one thing and wrong about another**, and a legged robot makes both worse: **every step shakes the accelerometer**. The skill is to **combine** them so that each is used where it is good. 19-09 and 25-07 did the same for a wheeled vehicle; the legs add the feet.

## The Concept

### The gyro: fast, and it drifts

Adding up the gyro's rates gives the angle, **accurate over a moment** and **wrong over a minute**, because its **bias** (here **0.5 degrees per second**, typical of a cheap gyro) is added up too: after **60 s** the gyro alone was **23.1 degrees** wrong (rms over the second half). See `E-LEG-0033`.

### The accelerometer: right on average, wrong at every step

At rest the accelerometer feels **gravity alone**, and the **arctangent of its components** is the roll and pitch. Walking, the body **accelerates** at every step and the accelerometer feels that too: shaken by **2.5 m/s2**, the accelerometer's angle alone was **10.2 degrees** wrong (rms). A **gate** on the size of what it feels (within 15 percent of g) did **not** help: it accepted **100 percent** of the readings, because a **sideways acceleration barely changes the magnitude** (the magnitude of gravity and a sideways push of 2.5 is 10.1, three percent above g): **the gate rejects jolts along the vertical, and not the sideways shake that fools the tilt**. See `E-LEG-0034`.

### The filter: add the gyro, pull gently

A **complementary filter** takes the best of each: **add the gyro's rate** each step (the fast part) and **pull gently toward the accelerometer's angle** (the slow part), by a gain k times the time step times the difference. A gain of **0.5 per second** averages the shake out (it is at 4 Hz, far above the filter's 0.08 Hz corner) and still removes the drift: the error fell to **1.02 degrees**. The **price** is a **steady offset** of about the bias over the gain (0.5 / 0.5 = 1 degree), which is what remains.

### The feet as a third opinion

In a **walk** three or more feet are on the ground, and 34-03's **plane through the feet** gives roll and pitch with **no drift** and **no shake** (its noise is about a degree). A second pull, toward the feet's angles, with a gain of **1 per second**, took the error to **0.40 degrees**: both pulls share the bias, and the feet's noise averages out. In a **trot** only **two** feet are down, **there is no plane**, the feet cannot be used, and the filter falls back to the gyro and the accelerometer: **1.02 degrees**, the same as without the feet. A gait with fewer feet down **needs better IMU handling**. See `E-LEG-0035`.

## Build It

`attitude_update` in `exercise/solution.hpp`.

```
rcpp verify 35-04
```

## Use It

`rc/legged/attitude.hpp`.

```cpp
rc::legged::AttitudeFilter f;
rc::legged::attitude_update(f, gyro_x, gyro_y, accel, feet_roll, feet_pitch, feet_down >= 3, dt);
```

**What this does not do.** It is **roll and pitch only**: the **heading** has no gravity to measure it by (a magnetometer or the legs' odometry would, 25). The **gyro's bias is not estimated**: it is left to the pull to remove (25-07 estimated it when the robot stood still, and a legged robot can do that between gaits). The **gains are fixed numbers**, with no change for the gait. The **accelerometer** is **assumed to be at the body's centre** and **to read without bias**. The **feet's angles** carry the **zeros' errors** of 34-08. The numbers are **typical**.

## What Breaks First

- **A gyro integrated alone.** See `E-LEG-0033`.
- **A gate on the size of the acceleration.** See `E-LEG-0034`.
- **A feet-based tilt used in a trot.** See `E-LEG-0035`.

## Ship It

`rc/legged/attitude.hpp` holds `AttitudeFilter`, `attitude_update` and `accel_angles`.
