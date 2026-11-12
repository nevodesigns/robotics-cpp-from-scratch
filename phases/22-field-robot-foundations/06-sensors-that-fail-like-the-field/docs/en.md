# Sensors That Fail Like the Field: Lidar, IMU, GNSS and Wheel Encoders, With Their Noise, Bias, Dropouts and Outliers

> A simulated sensor that is only noisy is a sensor that teaches nothing, because the field breaks sensors in other ways.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-02, 22-05, 21-10

## The Problem

A sensor model that adds Gaussian noise is the one that every controller in the world is tuned on, and the one that every field robot then fails on. The field breaks sensors in the other ways: a lidar beam that returns nothing, a gyro that is off by a constant and gets worse, a GNSS fix that is good to centimetres for a minute and jumps ten metres when a truck goes past, a wheel that spins on gravel and tells the odometry the robot moved. This lesson writes four sensors that fail the way the field makes them fail, so that the estimation and the safety of the later lessons are tested against something that fights back.

## The Concept

### Lidar

`cast_ray` finds the nearest of a set of walls (segments) and posts (circles) along a ray, within a range, and `scan` does it for a fan of beams from a pose. A beam to a wall at x = 5 m from a point 2 m off to the side, tilted 30 degrees, measured **5.7735 m**, which is `5 / cos(30 degrees)`: a range is not a distance to the wall, it is the distance along the beam. A beam that is too near, too far or parallel to a wall sees **nothing**, and says so with infinity, because a number such as 0 or the maximum range is a value the next stage will believe.

The scan then adds what the field adds: noise, and lost beams. From 40 scans of a room, measured against the noiseless scan, the noise was **0.0202 m** (0.02 set) and **0.90 percent** of the beams that should have returned did not (1 set). See `E-FIELD-0016`.

### IMU

A gyro's error has three parts: white noise that averages out, a bias that does not, and a bias that wanders. At rest with a bias of 0.01 rad/s, integrating the gyro gave **34.1 degrees** of heading in a minute. Noise is tuned out with a filter; bias has to be estimated, and it is the reason an IMU alone cannot be a compass (a later phase of this course estimates it). See `E-FIELD-0017`.

### GNSS

A receiver gives a position at three levels of quality. Alone, a position is good to metres; with corrections from a base station (RTK) it can be float, good to a quarter of a metre, or fixed, good to centimetres. The measured scatter was **1.49 m** per axis on a single fix and **0.020 m** on a fixed one. And about **1.03 percent** of fixes jumped by metres, which is multipath: the signal bounced off a wall and the receiver trusted it. A filter that models only the Gaussian part believes the jump. See `E-FIELD-0018`.

### Wheel encoder

An encoder counts whole ticks, so it is a tick short at worst: **1.00 mm** short over 10 m at 1000 ticks a metre. That is the least of its errors. A wheel radius wrong by 2 percent read **0.20 m** long over 10 m, and a wheel that slipped 5 percent (it turned more than the vehicle moved, on gravel or under acceleration) read **0.50 m** long. Neither shows as noise: they are errors in the same direction every time. See `E-FIELD-0019`.

## Build It

`cast_ray` in `exercise/solution.hpp`.

```
rcpp verify 22-06
```

## Use It

`rc/field/sensors.hpp` opens the module `rc::field`.

```cpp
rc::core::Rng rng(1);
rc::field::LidarSpec spec;
const std::vector<double> ranges = rc::field::scan(spec, world, x, y, theta, rng);
rc::field::Gnss gnss(rc::field::GnssSpec(), 5);
const rc::field::GnssFix fix = gnss.measure(x, y, rc::field::Fix::fixed);
```

**What this does not do.** The lidar is flat: it sees no ground, no overhang, no rain, and its noise does not grow with range or with the angle to the surface. The IMU is one gyro and two accelerometers with no temperature dependence and no cross axis error. The GNSS errors are independent from fix to fix where a real receiver's drift slowly, which is why a filter does better on the model than on the sky. And the values are typical of the class, not those of any one device.

## What Breaks First

- **A lost beam read as a range.** See `E-FIELD-0016`.
- **A gyro bias taken to be zero.** See `E-FIELD-0017`.
- **A GNSS jump believed.** See `E-FIELD-0018`.
- **An encoder trusted as the distance travelled.** See `E-FIELD-0019`.

## Ship It

`rc/field/sensors.hpp` holds `no_return`, `Segment`, `Circle`, `World2`, `cast_ray`, `LidarSpec`, `scan`, `ImuSpec`, `ImuSample`, `Imu`, `Fix`, `GnssSpec`, `GnssFix`, `Gnss`, `EncoderSpec` and `Encoder`.
