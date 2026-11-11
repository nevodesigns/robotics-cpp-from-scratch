# Two Sensors, One Angle: Fusing a Drifting Gyro With a Noisy Accelerometer

> One sensor is smooth and wanders off. The other is right on average and jittery. Believe each for the thing it is good at.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 15-01, 15-02

## The Problem

A joint's angle can be had two ways. A **gyro** reports how fast it is turning, and adding up that rate gives the angle. It is smooth, and every reading carries a small bias, so the sum walks away from the truth for ever.
An **accelerometer** reports which way gravity points, which gives the angle directly, with no walk, and every reading is noisy.

Lesson 15-02 showed what averaging cannot fix: a bias. A gyro is that problem in its purest form, since integrating a bias turns it into a growing error.

## The Concept

### What each does alone

A joint standing still at 0.3 rad, a 100 Hz gyro that reads 0.02 rad/s too high, and an accelerometer good to 0.05 rad, over 60 seconds, errors measured from the tenth second:

| source | rms error (rad) |
|---|---|
| gyro integrated alone | 0.764 |
| accelerometer alone | 0.050 |

A ramp of 0.02 rad/s over those seconds predicts 0.757 for the gyro. The gyro is better than the accelerometer for the first two seconds, then far worse. See `E-SENSE-0023`.

### A filter that hands over

The **complementary filter** takes the gyro's word for short times and the accelerometer's for long ones:

```cpp
alpha = tau / (tau + dt);
angle = alpha * (angle + gyro_rate * dt) + (1.0 - alpha) * accel_angle;
```

Each step, the gyro moves the estimate on, and a small share of the accelerometer's angle pulls it back. The **time constant** tau sets where the handover happens, at a frequency of 1 / (2 pi tau). `tau_for_crossover` goes the other way.

With tau of 1 s on the same still joint the error was 0.020 rad rms, and on a joint swinging half a radian either side every 5 seconds it was 0.021, against 0.761 for the gyro and 0.050 for the accelerometer. See `E-SENSE-0023`.

### What the bias still does

The pull toward the accelerometer is what holds the bias in check, and it does not remove it. With no noise at all, a still joint and a bias b, the filter settles to an error of b times tau:

| tau (s) | error (rad) | bias x tau |
|---|---|---|
| 0.25 | 0.0050 | 0.0050 |
| 0.50 | 0.0100 | 0.0100 |
| 1.00 | 0.0200 | 0.0200 |
| 2.00 | 0.0400 | 0.0400 |
| 4.00 | 0.0800 | 0.0800 |

A longer tau trusts the gyro more, so it lets more of the bias through. See `E-SENSE-0024`.

### The trade

A short tau passes the accelerometer's noise and a long one passes the gyro's bias and slows the filter down. On the swinging joint:

| tau (s) | crossover (Hz) | still, rms (rad) | swinging, rms (rad) |
|---|---|---|---|
| 0.05 | 3.183 | 0.0144 | 0.0144 |
| 0.20 | 0.796 | 0.0082 | 0.0082 |
| 0.50 | 0.318 | 0.0110 | 0.0111 |
| 1.00 | 0.159 | 0.0205 | 0.0205 |
| 2.00 | 0.080 | 0.0405 | 0.0407 |
| 5.00 | 0.032 | 0.0967 | 0.1001 |
| 20.00 | 0.008 | 0.2711 | 0.3202 |

The least error was at 0.2 s. Past it the bias takes over, and at 20 s the swing's own lag is added as well. No tau is best for a different bias or a different noise. See `E-SENSE-0025`.

## Build It

`complementary_alpha`, `tau_for_crossover` and `ComplementaryFilter::update` in `exercise/solution.hpp`. The class, `reset` and the accessors are given.

```
rcpp verify 15-11
```

The noise comes from a generator seeded in the test, so the figures above are the same on every run.

## Use It

`rc/sensor/complementary.hpp` joins `rc::sensor`.

```cpp
ComplementaryFilter filter(tau_for_crossover(0.5));
filter.reset(first_accel_angle);
// every sample:
const double angle = filter.update(gyro_rate, accel_angle, dt);
```

**What this does not do.** It fixes a bias in the sense of bounding it, to b times tau, and not of removing it. It assumes the accelerometer measures gravity alone, so a joint that is accelerating hard pulls the accelerometer's angle off and the filter follows
a little (lesson 15-07 deals with the sensor that is sometimes wildly wrong). It does not estimate the bias, which a Kalman filter does and phase 16 returns to, and it assumes the sample period is steady (lesson 15-03).

## What Breaks First

- **A gyro integrated alone.** It walks away at its bias per second. See `E-SENSE-0023`.
- **A tau chosen too long.** The bias comes back as b times tau. See `E-SENSE-0024`.
- **A tau chosen once, for all machines.** The least error depends on the noise and the bias. See `E-SENSE-0025`.

## Ship It

`rc/sensor/complementary.hpp` holds `complementary_alpha`, `tau_for_crossover` and `ComplementaryFilter`.

Lesson 15-12 turns to the other end of a sensor: the register it is read from.
