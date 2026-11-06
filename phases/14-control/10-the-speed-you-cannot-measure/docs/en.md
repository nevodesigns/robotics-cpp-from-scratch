# The Speed You Cannot Measure: Estimating It From Position and a Model

> A joint has an encoder that reports where it is. A damping term needs how fast it is going, and the
> obvious way to get that makes the noise a thousand times larger.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 14-04

## The Problem

A position loop that holds well rings when the load changes, and a derivative term is the cure: push back against the speed.
The speed has to come from somewhere, and a position sensor reports a position.

The obvious way is to subtract the last reading from this one and divide by the time between them. Every reading is wrong by a
little, and the error does not care whether the joint moved. Divided by a millisecond, a thousandth of a radian of error becomes a radian
a second.

The result is a speed trace that is mostly fuzz, a motor that buzzes at the noise's frequency and a derivative gain that has to be turned
down until it no longer does its job.

## The Concept

### Estimate it, do not compute it

An **observer** keeps its own idea of the position and the speed and checks it against the sensor. Each step has two parts:

1. **Predict.** If the joint was here going this fast, and the motor has just accelerated it by this much, it should now be there, going that fast.
2. **Correct.** Compare that prediction with the reading. The difference is the position error. Add a share of it to the position and a share of it to
   the speed.

```cpp
predicted_position = position + velocity * dt + 0.5 * acceleration * dt * dt;
predicted_velocity = velocity + acceleration * dt;
error = measured - predicted_position;
position = predicted_position + gains.position * dt * error;
velocity = predicted_velocity + gains.velocity * dt * error;
```

The noise only enters through the correction, and the correction is small each step, so most of what the estimate does is follow the model.

### One number sets both gains

The two shares are not independent. Place both poles of the observer at minus w and the gains are 2 w and w squared, the expansion of
(s + w)^2. That is a critically damped pair, which settles as fast as w allows without ringing. `observer_gains(w)` returns them, and w is the
**bandwidth** in radians a second.

Started with the speed wrong by 2 rad/s and a noise free sensor, at w of 50 the error was 0.0847 after a tenth of a second and under a millionth after one.

### The difference against the observer

A swing of half a radian either side at 1 Hz, with a sensor good to 0.001 rad read at 1 kHz:

| method | speed error, rms |
|---|---|
| difference of two readings | 1.422 rad/s |
| observer, bandwidth 50, told the acceleration | 0.006 rad/s |

The observer is over 200 times quieter, with the same sensor and the same readings. See `E-CTRL-0024`.

### The model matters

The acceleration input is what lets the estimate follow a change in speed without waiting for the position to be wrong. Without it, the prediction assumes
the speed stays as it is, and every change arrives late, through the correction.

On a noise free swing at a bandwidth of 50, told the acceleration the speed error was 1.70e-03 rad/s, and told zero it was 0.543. See `E-CTRL-0025`.

The model does not have to be exact. A push it does not know about, an acceleration d the observer was told nothing of, leaves a steady speed error of
**2 d / w**. For d of 1 rad/s^2 at w of 50 that predicts 0.0400 and the test measured 0.0395.

### One bandwidth, two errors

A higher w follows the truth more closely, including what the model got wrong, and passes more of the noise.

| bandwidth | noise only | tracking only | both |
|---|---|---|---|
| 5 | 0.0002 | 2.5516 | 2.5515 |
| 20 | 0.0013 | 1.2771 | 1.2770 |
| 50 | 0.0053 | 0.5433 | 0.5433 |
| 100 | 0.0159 | 0.2711 | 0.2715 |
| 200 | 0.0490 | 0.1324 | 0.1412 |
| 400 | 0.1675 | 0.0628 | 0.1789 |

Noise rises with w, the lag falls with it, and with no acceleration input the two together are smallest near 200 of these values. See `E-CTRL-0026`.

## Build It

`observer_gains` and `VelocityObserver::update` in `exercise/solution.hpp`. The class, `reset` and the accessors are given.

```
rcpp verify 14-10
```

The tests make their own noise from a generator seeded in the test, so the numbers above are the same on every run.

## Use It

`rc/control/observer.hpp` joins `rc::control`.

```cpp
VelocityObserver observer(observer_gains(50.0));
observer.reset(encoder_position, 0.0);
// every control step:
observer.update(encoder_position, torque / inertia, dt);
const double torque_out = kp * (target - observer.position()) - kd * observer.velocity();
```

**What this does not do.** It is a fixed gain observer for one joint with a known inertia. It does not choose its own gains from the noise, which is what a Kalman filter does
and phase 16 returns to; it has no idea of the sensor's resolution, so a coarse encoder at low speed still gives it a staircase to follow (lesson 15-09); and it assumes the timestep is
steady. A model that is wrong in a way that changes, friction for one (lesson 14-08), leaves a speed error it cannot see.

## What Breaks First

- **Speed as a difference of readings.** The sensor's noise times 1 / dt. See `E-CTRL-0024`.
- **An observer given no acceleration.** Smooth and late. See `E-CTRL-0025`.
- **One bandwidth for noise and tracking.** It cannot be both quiet and quick. See `E-CTRL-0026`.

## Ship It

`rc/control/observer.hpp` holds `ObserverGains`, `observer_gains` and `VelocityObserver`.

Lesson 14-09 took a speed as a given. This is where it comes from when nothing measures it.
