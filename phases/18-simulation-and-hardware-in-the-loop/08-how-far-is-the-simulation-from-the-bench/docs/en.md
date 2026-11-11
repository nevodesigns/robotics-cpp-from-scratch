# How Far Is the Simulation From the Bench: Comparing Two Traces, and the Noise Floor No Model Can Beat

> "It looks about right" is how every simulation that was wrong was first described.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 18-03, 18-04, 15-02, 03-02

## The Problem

By lesson 18-04 the simulator has a motor measured from the real one, a sensor with the real one's faults, and a controller that runs on either. The question that decides whether any of it is worth trusting is: **how different is the simulation from the bench?** The usual answer is to plot the two traces on the same axes and look. That finds a gross error and misses a small one that matters, and it cannot be put in a test.

This lesson turns the look into numbers: how big the typical difference is, how big the worst one is, whether the simulation sits above or below, whether it is early or late. It also gives the number that says when to stop improving the model, because a measurement has noise, and a model cannot be closer to a noisy measurement than the noise.

## The Concept

### Four numbers

`compare` takes a simulated trace and a real one and gives:

- the **rms error**: the square root of the mean of the squared differences, the typical size of the miss;
- the **largest** single difference;
- the **bias**: the mean difference, which says whether the simulation sits high or low;
- the **lag**: how many samples later the real trace is than the simulation, found by trying shifts and keeping the best fit.

A trace that was 0.05 too high everywhere gave an error of 0.0500, a largest difference of 0.0500, a bias of 0.0500 and a lag of 0. A single sample out by a whole unit in 200 gave a largest difference of 1.0 and an rms error of 0.0707, the square root of 1/200: the rms error is how a rare big miss is made small, and the largest difference is how it is kept in view. See `E-SIM-0022`.

### Why lag is judged by variance

The best shift is the one for which the traces fit best, and "best" must not be fooled by the bias. With a constant offset between the traces, the shift that gives the smallest plain error is not the true one, since shifting a rising trace by a sample also moves it nearer to a trace that is too high. So each shift is judged by the **variance** of the differences, how much they vary, with the constant taken out. The 0.05 offset gave a lag of 0 and not 1.
See `E-SIM-0022`.

### Late is not the same as slow

A motor's speed read through a sensor that is 5 samples late, with the motor driven open loop so that nothing reacts to the sensor:

```
error lined up 0.4042, best shift 5 samples, error at that shift 0.0000
```

A pure delay is found exactly: 5 samples, and once the real trace is shifted back by them the error is nothing. Which sign is which is in the answer: compared the other way round the lag was -5. A trace delayed by 7 was found at 7.

In a **closed loop** it is different. The same 5 samples of sensor delay, inside the controller's loop, gave:

```
error lined up 0.1553, best shift 4, error at that shift 0.1396
```

The best shift was 4, and shifting barely helped: 0.1396 against 0.1553. A late sensor in a loop is not a late copy of the same trace, since the controller reacts to the late reading and the motor does something different. The lag number is a diagnosis for an open loop signal. In a closed loop it says "this is late" and not by how much of the trouble is delay. See `E-SIM-0023`.

### The noise floor

The real trace is noisy, so even a perfect model does not match it. Two real runs of the same thing differ only by noise, the difference of two noisy numbers has twice the variance of one, and the **noise floor** is the rms difference divided by the square root of two. With sensor noise of 0.01, the floor was **0.0102**, and a perfect noiseless model scored an error of **0.0105** against a real run: right on the floor.

A model cannot score below it, and a model that scores on it is as good as the measurement lets it be. Improving it further is fitting noise. The question for a simulation is not "is the error zero" but "how far above the floor is it". See `E-SIM-0023`.

### How wrong a model is, as a number

A real motor with a time constant of 0.12 s and a noise floor of 0.0097, against models with other time constants, in the same position loop:

| model's time constant (s) | rmse | above the floor |
|---|---|---|
| 0.05 | 0.0810 | 0.0713 |
| 0.08 | 0.0501 | 0.0404 |
| 0.10 | 0.0272 | 0.0175 |
| **0.12** | **0.0098** | **0.0001** |
| 0.15 | 0.0374 | 0.0277 |
| 0.20 | 0.0930 | 0.0833 |
| 0.30 | 0.1822 | 0.1725 |

The error is smallest at the true time constant, where it sits on the floor, and grows both ways. A model of 0.10, 17 percent out, is already nearly three times the floor. The loops look much the same on a plot. The numbers do not, and the minimum of that column is also a way to find the time constant (lesson 18-03). See `E-SIM-0023`.

### Limits

A simulation is acceptable if it stays within limits, and `exceeded` names the ones it did not. Limits of an rms error of 0.02, a largest difference of 0.05 and a lag of 1 sample:

| model | rmse | largest | lag | exceeded |
|---|---|---|---|---|
| fitted to the bench (the right motor, the sensor delay) | 0.0048 | 0.0141 | 0 | none |
| the wrong motor and no sensor delay | 0.1243 | 0.3657 | 6 | rmse max lag |

The names tell a person what to look at: a model that exceeds only `lag` is missing a delay, one that exceeds only `rmse` and `max` has the wrong dynamics. A limit that is not exceeded is a statement that can go in a test, and the test fails the day the motor is replaced and nobody updated the model. See `E-SIM-0024`.

## Build It

`compare`, `noise_floor` and `exceeded` in `exercise/solution.hpp`. The types are given.

```
rcpp verify 18-08
```

The noise comes from generators seeded in the test, so the figures above are the same on every run.

## Use It

`rc/sim/compare.hpp` joins `rc::sim`.

```cpp
const rc::sim::Metrics m = rc::sim::compare(sim_trace, bench_trace, 20);
const double floor = rc::sim::noise_floor(bench_run_1, bench_run_2);
const auto bad = rc::sim::exceeded(m, rc::sim::Limits{0.02, 0.05, 1});
```

**What this does not do.** It compares one signal at a time on a common clock: two traces recorded at different rates must be put on the same instants first (lesson 15-15). It uses whole samples of lag, which for a 10 ms trace is coarse. It assumes the noise on the two real runs is independent and the same, which a sensor that drifts from run to run is not: then the floor is too high.
It says how different the traces are and not why. And limits come from a decision about what the controller needs, a number the simulation cannot supply.

## What Breaks First

- **A comparison by eye.** A model that is 17 percent out and looks fine. See `E-SIM-0023`.
- **A lag found on a trace with an offset.** A shift of one where there is none. See `E-SIM-0022`.
- **A model improved below the noise floor.** Fitting the noise. See `E-SIM-0023`.
- **No limits.** A simulation nobody can say has stopped matching. See `E-SIM-0024`.

## Ship It

`rc/sim/compare.hpp` holds `Metrics`, `compare`, `noise_floor`, `Limits` and `exceeded`.

The distance between the simulation and the bench is now a number with a floor under it and a line over it.
