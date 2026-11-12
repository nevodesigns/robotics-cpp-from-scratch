# Odometry for Each Kind of Vehicle: Dead Reckoning From Wheels for a Differential Drive, a Skid Steer and a Car, and the One Parameter Each Is Most Sensitive To

> Every wheel odometry is right when the model is right, and the question is which wrong number costs the most.

**Type:** Build
**Time:** about 85 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 23-01, 23-02, 23-03, 21-10

## The Problem

A robot that knows how far each wheel has rolled can work out where it is. That is wheel odometry, and it is the cheapest sensor a ground robot has and the one every other sensor is first compared with. How wrong it is depends on the vehicle, and on **which of its numbers is wrong**. A differential drive and a skid steer share a formula and differ in one number. A car has a different formula whose weakest point is a sensor that is not on the wheels at all.

## The Concept

### The vehicles that steer with their sides

`WheelOdometry` takes the distance the wheels of each side rolled in a step. The distance moved is the mean of the two, times a **scale** (one less the slip, or a calibrated wheel radius); the turn is the difference over an **effective track**: the real track times the ICR coefficient for a skid steer, or divided by the turn efficiency, as in lesson 23-02, which absorbs everything else that makes a turn other than the wheels say. It integrates on the heading at the middle of the step. With the parameters of the model it **was** the model, to 1e-9, and the square of 10 m sides closed within **0.023 m**.

### Which wrong number costs the most

A track width **3 percent too small** made a full turn in place read **11.1 degrees too much**: the error is the same fraction of every turn, so a robot that turns a lot loses its heading fast. A wheel scale **2 percent out** made 10 m read **0.20 m long**: the same fraction of every metre. Neither is noise: both are the same error every time, in the same direction, and a calibration can take them out (the next lessons do). See `E-VEH-0019`.

### The ideal parameters, against the losses of a real floor

Drive the square with a model that has a slip of 2 percent and a turn efficiency of 0.9 (lesson 23-01). The odometry with the ideal parameters believed it ended **0.02 m** from home. The robot was **4.6 m** from home. The ideal odometry is **consistent with itself and wrong about the world**, which is the worst kind of wrong. With the effective track (the track over the efficiency) and the scale (one less the slip), the odometry's belief differed from the truth by only **0.000 m**. See `E-VEH-0020`.

### The car's odometry is in its steering sensor

For a car the rear wheels give the distance and the **steering angle** gives the turn: `dtheta = ds * tan(delta) / L`. The sensor that measures `delta` has a zero, and if the wheels point straight when it reads one degree, the odometry thinks the car is turning. A car driven **straight for 20 m** with a sensor one degree off believed it had turned **61 degrees** and moved **9.6 m sideways**. A one degree error in a number nobody looked at is a robot that thinks it is in a different part of the room. The cure is to find the zero of the steering with the car on a straight line, which is the first thing to calibrate on a car. See `E-VEH-0021`.

## Build It

`WheelOdometry::update` in `exercise/solution.hpp`.

```
rcpp verify 23-07
```

## Use It

`rc/vehicle/odometry.hpp` opens nothing new in `rc::vehicle`.

```cpp
rc::vehicle::WheelOdometry odo(track / efficiency, 1.0 - slip);
odo.update(left_wheel_metres, right_wheel_metres);
rc::vehicle::AckermannOdometry car(wheelbase, steering_zero);
car.update(rear_wheel_metres, steering_sensor_reading);
```

**What this does not do.** It takes the wheel travel as measured, with no noise and no quantisation (lesson 22-06 added both). The losses are constants, and on a real floor they change with the floor. It does not fuse the odometry with anything, which a later phase of this course does, and an odometry without that is not a position: it is a running total of mistakes. And a tracked vehicle needs the slips of its two tracks, which wheels on the ground do not give.

## What Breaks First

- **A track width taken from a ruler.** See `E-VEH-0019`.
- **Odometry that is consistent and wrong.** See `E-VEH-0020`.
- **A steering sensor with an unmeasured zero.** See `E-VEH-0021`.

## Ship It

`rc/vehicle/odometry.hpp` holds `WheelOdometry` and `AckermannOdometry`.
