# The Noise the Derivative Loves: A Filter on the D Term and What It Costs

> A derivative term is the part of a controller that reacts to how fast the error is changing. It cannot tell a change from a flicker.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 14-01, 14-10

## The Problem

Lesson 14-01 gave the loop a derivative term and took it from the measurement, so a new target did not kick it. That fixed a step in the setpoint. It did nothing about
the sensor, which is wrong by a little on every reading, and a derivative divides that little by the time between readings.

With a sensor good to 0.001 rad read at 1 kHz, the derivative of a joint standing perfectly still came out at 1.4067 rad/s rms. Multiply by a gain of 4 and the motor is asked for
a force that jumps by several newtons from one step to the next, while the joint goes nowhere.

Lesson 14-10 removed that noise by estimating the speed from a model. This lesson is the cheaper fix, for a loop that has no model to lean on: smooth the derivative.

## The Concept

### A first order low pass

Instead of using each new raw derivative, move the stored value part of the way toward it.

```cpp
raw = (measurement - last) / dt;
value += alpha * (raw - value);
```

`alpha` is the share of each new value let in. A small alpha is a slow filter. It is set from a **time constant** tf in seconds:

```cpp
alpha = dt / (tf + dt);
```

After one time constant a steady slope of 3 rad/s read 1.869, about 63 percent of the way, and after ten it read 3.000. A tf of zero gives an alpha of one and the raw derivative back. See `E-CTRL-0027`.

### What it buys

The derivative of sensor noise alone, rms:

| tf (s) | rms (rad/s) | share of raw |
|---|---|---|
| 0.000 | 1.4067 | 1.000 |
| 0.005 | 0.1737 | 0.123 |
| 0.010 | 0.0928 | 0.066 |
| 0.020 | 0.0480 | 0.034 |
| 0.050 | 0.0196 | 0.014 |

A time constant of 0.02 s, twenty samples, left under 4 percent of the noise. See `E-CTRL-0027`.

### What it costs

A filter that smooths the noise smooths everything, and a real change in the derivative arrives late by about tf. On a noise free swing of half a radian at 1 Hz:

| tf (s) | rms error (rad/s) |
|---|---|
| 0.000 | 0.0000 |
| 0.005 | 0.0697 |
| 0.010 | 0.1393 |
| 0.020 | 0.2769 |
| 0.050 | 0.6652 |

The error grows with tf, almost in proportion. The noise falls and the lag rises, and there is no setting where both are zero. See `E-CTRL-0028`.

### In the loop

A position loop with kp 40 and kd 4, a unit mass and the same noisy sensor, and the change in the force from one step to the next:

| derivative | force step, rms | error at the end |
|---|---|---|
| no filter | 9.373 N | -0.0001 rad |
| tf 0.02 s | 0.317 N | 0.0000 rad |

Thirty times quieter, and the joint still arrives. See `E-CTRL-0029`.

### Choosing tf

A common rule ties it to the derivative time, tf = Td / N, with N between 5 and 20 and Td = kd / kp. Here Td is 0.1 s, so an N of 5 is 0.02 s. A larger N is a faster filter and passes more noise.
The derivative is only useful over the range of frequencies the joint moves at, so the filter has to stay well above them.

## Build It

`filter_alpha` and `DerivativeFilter::update` in `exercise/solution.hpp`. The class, `reset` and the accessors are given.

```
rcpp verify 14-11
```

Every noise sequence is made by a generator seeded in the test, so the numbers above are the same on every run.

## Use It

`rc/control/filtered_derivative.hpp` joins `rc::control`.

```cpp
DerivativeFilter derivative(0.02);
// every control step:
const double rate = derivative.update(encoder_position, dt);
const double force = kp * (target - encoder_position) - kd * rate;
```

**What this does not do.** It is one pole, so it delays everything by about tf, and it does not know whether a jump was noise or a real knock. It does not replace the observer of lesson 14-10, which keeps its lag low by using the
commanded acceleration. It also does nothing about a sensor that moves in steps, where the derivative is zero or a spike (lesson 15-09), and the loop's own delay (lesson 14-07) adds to the filter's.

## What Breaks First

- **A raw derivative on a noisy sensor.** The noise times 1 / dt, and a buzzing motor. See `E-CTRL-0027`.
- **A filter chosen for quiet.** It lags the real motion by about tf. See `E-CTRL-0028`.
- **A derivative gain turned up with no filter.** The chatter scales with it. See `E-CTRL-0029`.

## Ship It

`rc/control/filtered_derivative.hpp` holds `filter_alpha` and `DerivativeFilter`.

Lesson 14-12 asks a different question about the same loop: how often does it have to run.
