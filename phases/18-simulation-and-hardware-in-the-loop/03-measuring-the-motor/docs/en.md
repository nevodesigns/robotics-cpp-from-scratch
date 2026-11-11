# Measuring the Motor: Finding Gain and Time Constant From a Step, and Knowing How Far to Trust Them

> The datasheet says the time constant is 0.1 seconds. The datasheet is not the motor on your bench, warmed up, with a gearbox on it.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 18-02, 14-04, 15-02

## The Problem

Lesson 18-02 ended on a motor model with two numbers in it, a gain and a time constant, and the remark that a model that keeps every rule of the contract can still be the wrong motor. The numbers have to come from somewhere, and the place is the machine: command a step, record the speed, and find the two numbers that make the model's curve lie on the measured one. That is **system identification**, in its smallest honest form.

It is also where a simulation's credibility is decided. A controller tuned on a model with a time constant of 0.05 s on a motor whose is 0.1 s will overshoot on the real thing, and the simulation will have promised it would not.

## The Concept

### The model

A step of `command` from rest gives a speed of

```cpp
gain * command * (1.0 - std::exp(-t / tau))
```

zero at the start, 63 percent of the final speed after one `tau`, and the final speed, gain times command, in the end. The **gain** is how much speed a unit of command buys, and the **time constant** is how long the motor takes to respond. `rms_error` is the number that says how far a model's trace is from a measured one: the root mean square of the differences. See `E-SIM-0009`.

### Reading the numbers off a trace

The gain is easy: the speed the motor settled to, over the command, taking the mean of the last tenth of the readings so that one noisy sample does not decide it. The time constant is harder. If the speed is `final * (1 - exp(-t / tau))`, then

```
ln(1 - speed / final) = -t / tau
```

which is a straight line in t through the origin with a slope of -1/tau. A line fitted by least squares through the readings that are between 5 and 90 percent of the way to the final speed gives tau. Near the top the logarithm is of a number close to zero, which is mostly noise, so those readings are left out, and below 5 percent the speed has hardly moved.

A clean step of 3 on a motor of gain 2 and time constant 0.1 s:

```
found gain 1.9991, time constant 0.0989 s, residual 0.00208
```

The time constant came out at 0.0989 and not 0.1000, which is not the fit's error. It is the motor's: the simulated motor steps with Euler's method at 2 ms (lesson 18-01), and a first order system stepped that way settles as though its time constant were about half a step shorter. A measurement of a simulation measures the simulation. See `E-SIM-0009`.

### Noise leans the line

Real readings are noisy. 200 noisy runs of the same step at each level of noise, the time constant found, the true one being 0.1:

| noise | quick fit | refined | read off the 63 percent crossing |
|---|---|---|---|
| 0.05 | 0.0992 (0.0005) | 0.0989 (0.0004) | 0.0994 (0.0019) |
| 0.30 | 0.1483 (0.0148) | 0.0989 (0.0027) | 0.0876 (0.0070) |
| 0.50 | 0.2221 (0.0113) | 0.0989 (0.0044) | 0.0794 (0.0091) |

The numbers in brackets are the spread from run to run. At a noise of 0.5 the quick fit said **0.222**, more than twice the truth, with a small spread: it is confidently wrong. The reason is the logarithm: the average of the logarithm of a noisy number is not the logarithm of the average, so noise pushes every point the same way and the line leans. Reading the 63 percent crossing leans the other way, as noise makes the speed cross early: 0.0794.

The remedy is to stop transforming the data. The **refined** fit keeps the gain and searches directly for the time constant that makes the model's rms error against the readings smallest, by golden section search between a third and three times the quick estimate. It gave 0.0989 at every noise level, the same as the clean answer, with a spread of 0.0044 at the worst. Use the quick fit to get started and the direct search to get the answer. See `E-SIM-0007`.

### How far to trust it

A model fitted on a step of 1, whose speed is 2 and well inside the motor's range, and asked to predict other steps:

| command | rms error of the prediction (speed units) |
|---|---|
| 2 | 0.0014 |
| 4 | 0.0028 |
| 6 | 0.0042 |
| 12 | 0.0083 |
| 20 | 14.4390 |

Up to a command of 12, the motor's limit, the model predicts to within 0.01 of a speed that reaches 24. At 20 the real motor is clamped to 12 and goes to 24 where the model says 40: an error of 14.4. The lesson is the one about extrapolation: a model is good where it was fitted and the limit is a part of it, not found by the fit. The gain and time constant describe the linear range. See `E-SIM-0008`.

### What cannot be fitted

A trace is refused rather than turned into numbers when it cannot give any: fewer than 10 readings, traces of different lengths, a command of zero, a motor that never moved, or a speed that is already at its final value from the first reading, so that no part of the rise was seen. A fit that returned something for these would be a number with no meaning attached to it. See `E-SIM-0009`.

## Build It

`predict`, `rms_error`, `fit_first_order` and `refine_first_order` in `exercise/solution.hpp`. The `Fit` type is given.

```
rcpp verify 18-03
```

The noise comes from a generator seeded in the test, so the figures above are the same on every run.

## Use It

`rc/sim/identify.hpp` joins `rc::sim`.

```cpp
const rc::sim::Fit f = rc::sim::refine_first_order(times, speeds, command);
if (f.ok) rc::sim::SimMotor motor(f.gain, f.tau, measured_limit);
```

**What this does not do.** It fits a first order lag, and a motor with a gearbox has backlash and a second time constant that this does not see. It fits one direction of one step: a motor that is quicker to speed up than to slow down needs two. It cannot tell a noisy sensor from a motor that varies. It takes the limit and any dead band as given: a command below the dead band does nothing on a real motor and that is not in
the model. And the fit is of the plant as it was on that day, warm or cold, loaded or not; a model made on the bench is the bench's.

## What Breaks First

- **A log fit on noisy data.** Confidently wrong, by a factor of two at a noise of 0.5. See `E-SIM-0007`.
- **A model used beyond what was fitted.** An error of 14 at a command the motor clamps. See `E-SIM-0008`.
- **A fit with no check on the trace.** Numbers from a motor that never moved. See `E-SIM-0009`.

## Ship It

`rc/sim/identify.hpp` holds `Fit`, `predict`, `rms_error`, `fit_first_order` and `refine_first_order`.

The numbers in the motor model now come from a measurement, with an error bar.
