# A Robot That Balances: An Inverted Pendulum, a Filter, a Delay It Cannot Afford, and a Stop That Stops It Running Away

> A robot that balances has no safe state to stand in. The only safe thing it can do when it fails is fall over without driving anywhere.

**Type:** Capstone
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 21-03, 19-14, 19-09, 19-10, 15-11, 14-01, 14-07

## The Problem

The fourth capstone is the one that is **unstable**. A line follower, a rover and an arm are all robots whose safe state is to stand still. A robot on two wheels, balancing like a broom on a hand, is falling at every moment and is kept up only by the control loop. That makes it the best test of everything the curriculum said about sensors, filters, delay and safety, because each of them is the difference between standing and a fall.

The pieces are the pendulum model, the complementary filter of lesson 15-11, a PD controller with a velocity loop (14-01), the delay results of 14-07, and the tilt monitor and supervisor of 19-09 and 19-10.

## The Concept

### The model, and the sum that is only a bound

The body is a pendulum of length 0.15 m above the axle. With the axle's acceleration `a`, the body's angular acceleration is `(g sin(theta) - a cos(theta)) / L`. A lean is held when `a = g tan(theta)`; the test checks that this gives zero. So the steepest lean a motor limit `a_max` could hold is `atan(a_max / g)`: **22.2 degrees** for 4 m/s^2. It is an upper bound, and measured it is nowhere near:

| a_max | steepest lean recovered | atan(a_max / g) |
|---|---|---|
| 2 m/s^2 | 8.9 degrees | 11.5 degrees |
| 4 | **16.5** | 22.2 |
| 8 | 21.7 | 39.2 |

The measured limit is below the sum at every row, and at 8 m/s^2 it is only **55 percent** of it, because the axle's own speed limit (1.5 m/s) then bites: the wheels cannot accelerate past it. The controller also needs margin that a formula does not: it has to push harder than the hold, to bring the body back. The formula says what is **impossible**; the simulation says what is **achieved**. See `E-CAP-0010`.

### The control law

`balance_command` is a PD controller with a velocity loop: the aim is a lean of **minus** `kv` times the speed (limited to 0.1 rad), so that a robot going forward aims to lean back, and the command is `kp` times (lean minus aim) plus `kd` times the lean rate. `kp` is 25, above g, so that it pushes harder than gravity does. Going forward at 1 m/s the command is +1.25: the axle is pushed **forward first**, to tip the body back, and the body then holds a backward lean that slows the robot. A robot that balances is a non-minimum phase system: to go back it must first go forward, and that is why the first answer to "slow down" is the wrong way.

### A delay it cannot afford

A delay between the filter and the wheels. The body falls with a time constant of `sqrt(L / g)`, **0.124 s**; a delay that is a good part of that is more than the loop can recover from:

| delay | result |
|---|---|
| 0, 20, 40, 50 ms | stands (rms lean 0.0092, 0.0096, 0.0107, 0.0164) |
| 55 ms | **FELL** at 2.79 s |
| 60 ms | FELL at 1.55 s |
| 70 ms | FELL at 1.04 s |

Standing at 50 ms and falling at 55 is not a gradual loss: it is an **edge**, and the rms lean at 50 ms (0.0164) is already **1.8 times** that with no delay. A balancing robot is usually in the same state as at 50 ms: working, with 10 percent of margin it does not know it has. This is lesson 14-07's result in a place where the answer is a fall. See `E-CAP-0011`.

### The filter

The accelerometer measures the lean and also the axle's own acceleration (it cannot tell them apart), and is noisy; the gyro is smooth and has a **bias**. The complementary filter of 15-11 joins them with a time constant `tau`. With a gyro bias of 0.05 rad/s and accelerometer noise of 0.05 rad, over 20 s:

| tau | result |
|---|---|
| 0.05 s | **FELL at 1.15 s** (the noise gets to the wheels) |
| 0.2 s | stands, **drifted -4.87 m** |
| 0.5 s | stands, drifted -9.38 m |
| 2 s | FELL at 7.77 s |
| 5 s | FELL at 4.71 s |
| 1000 s (the gyro alone) | **FELL at 3.92 s** |

Too quick and the accelerometer's noise is a command to the wheels; too slow and the bias builds up as an error in the lean the robot believes it has. In the middle it **stands, and walks away**: the bias leaves a small lean that the robot thinks is upright, so it holds a lean and accelerates until the velocity loop settles, which is a drift of 5 to 9 metres in 20 s. Standing is not the same as being right, and the drift is the part of the problem no filter fixes: it takes a **second sensor on the wheels' position** (the encoders), which is why the velocity loop is there at all. See `E-CAP-0011`.

### When it cannot be caught

Started 25 degrees over, which cannot be recovered. With the supervisor's tilt stop at 25 degrees estimated, the motors are cut at 0.195 s; the cause is `tilt`. The axle went **0.54 m**, at most **0.69 m/s**, and **0.45 m** while the robot fell. Without it the controller goes on trying, pushing the axle to its top speed: **3.17 m**, **1.51 m/s**, and **3.07 m while it fell**. A fallen robot that drives across a room at 1.5 m/s is a worse hazard than one that fell, and the stop is what separates them. It does not save the robot (a balancing robot has no safe state to stand in); it limits the harm to a fall.

The IMU that goes dead at 2 s: with the supervisor, a stop at **2.010 s** with the cause `sensors`; without, no stop and **no cause**, and it falls at 2.81 s all the same. The fall cannot be avoided, but the log can say why. See `E-CAP-0012`.

## Build It

`pendulum_accel`, `max_recoverable_tilt` and `balance_command` in `exercise/solution.hpp`. The filter, sensors, delay, limits and supervisor wiring are given.

```
rcpp verify 21-04
```

## Use It

`rc/capstone/balancer.hpp` joins `rc::capstone`.

```cpp
BalanceConfig config;
config.filter_tau = 0.5;
config.delay_frames = 4;
const BalanceResult r = run_balance(config, 20.0);
if (r.stopped) report(r.causes, r.stopped_at);
```

## Show It

The table to show is the **delay**: it stands at 50 ms and falls at 55, a number that a person designing the real robot can use on the day the camera or the bus is changed. Show the filter table with the drift column, which is the result that surprises: it stands and it walks away. And show the **two lines of the stop**: 0.54 m against 3.17 m. A balancing robot is a good portfolio piece because every one of those numbers has a physical meaning.

**What this does not do.** It is a planar linear-ish model with a point mass: no wheel slip, no motor dynamics (the command is an acceleration, instantly), no cart friction beyond a rolling term, no steering, no slope (lesson 19-09), and a gyro bias that is constant. The controller is two gains and a velocity loop, with none of the tuning of lesson 14-01, an LQR or a state estimator for position. The supervisor stops the motors at 25 degrees estimated, and a real robot would try to fall **safely** (a leg that drops, a tail, a soft landing), which is a design matter and not code. The tilt monitor uses the estimate, which is what the robot believes, and a robot with a bad filter believes it is upright when it is not. And the numbers are for a particular length, gain and noise: the structure is what carries over, not the 55 ms.

## What Breaks First

- **A formula taken for the achievable.** See `E-CAP-0010`.
- **A delay that nobody measured, working just inside its edge.** See `E-CAP-0011`.
- **A filter chosen from one of its two failures.** See `E-CAP-0011`.
- **A stop that was never tried at a lean that cannot be caught.** See `E-CAP-0012`.

## Ship It

`rc/capstone/balancer.hpp` holds `pendulum_accel`, `max_recoverable_tilt`, `balance_command`, `BalanceConfig`, `BalanceResult`, `BalanceNoise` and `run_balance`.
