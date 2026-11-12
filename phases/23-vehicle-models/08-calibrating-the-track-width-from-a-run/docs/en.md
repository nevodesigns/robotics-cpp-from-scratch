# Calibrating the Effective Track Width From a Run: Spinning in Place Both Ways to Cancel the Gyro's Bias, and How Many Spins Are Enough

> The number that says how much the robot turns can be measured in a minute, as long as the instrument that measures the turn is made to cancel its own error.

**Type:** Build
**Time:** about 80 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 23-07, 23-01, 22-06, 21-10

## The Problem

The effective track width is the number that sets how much a skid steer robot turns for a given difference of its wheels, and it is not on any data sheet: it depends on the floor, the tyres and the load. It has to be measured, and the way to measure it is easy to state: spin the robot in place and compare how far apart its wheels rolled with how far it turned. The wheels give the first at no cost. The second needs an instrument, and an instrument has its own errors; a gyro's is a **bias**, a constant that adds to every reading, and it is large enough to matter: 0.01 rad/s over a spin of six seconds is 0.06 rad of heading that never happened.

## The Concept

### The ratio

In a spin the right wheels roll further than the left by an amount that, over the whole turn, is `effective_track * heading_change`. So the track is the total wheel difference over the total heading change. `calibrate_effective_track` adds up the absolute wheel differences and the absolute heading changes over a set of runs and divides the first by the second. With a perfect gyro, a single spin gave the effective track of the vehicle (0.2051 m for an efficiency of 0.78 on a track of 0.16 m) to 2e-4, and it did not matter which way the robot spun. See `E-VEH-0022`.

### Cancelling the bias

A gyro's bias adds to the reading while the robot turns one way and subtracts from its size while it turns the other: a spin to the left reads too much and a spin to the right reads too little by the same amount. Taking the sizes and adding them cancels it. With a bias of 0.01 rad/s, a spin one way gave an error of **0.31 percent** in the track and a spin each way **0.000 percent**. Always calibrate with runs in both directions: it is the cheapest correction there is, and it needs no model of the bias at all. See `E-VEH-0023`.

### How many spins

Noise in the gyro is not cancelled, only averaged. Thirty calibrations with a noisy gyro (0.05 rad/s of noise in a reading, a bias of 0.01) gave a spread of **0.0034 m** in the track with one spin each way and **0.0019 m** with four spins each way, narrower by the amount that taking the average of more runs gives. The mean of both was within a few millimetres of the truth. The cost of a calibration is a minute of the robot's time; the cost of a bad one is every turn after it. See `E-VEH-0024`.

### The scale

The distance scale is easier still, and needs a tape: drive a stretch, measure it, read the wheel travel. A stretch of 10.00 m that the wheels counted as 10.21 m gave a scale of **0.9794**. `spread_of` is the mean and standard deviation that make the comparison above.

## Build It

`calibrate_effective_track` in `exercise/solution.hpp`.

```
rcpp verify 23-08
```

## Use It

`rc/vehicle/calibrate.hpp` opens nothing new in `rc::vehicle`.

```cpp
std::vector<rc::vehicle::TurnRun> runs;
runs.push_back(spin_left_and_measure());
runs.push_back(spin_right_and_measure());
const double track = rc::vehicle::calibrate_effective_track(runs);
rc::vehicle::WheelOdometry odo(track, scale);
```

**What this does not do.** It assumes the robot turns in place about its middle and that the efficiency is the same at every speed and every turn, which on a real floor it is not (it changes with the turn rate and with the surface). The calibration is for one floor, so a robot that changes floors needs a calibration for each or a model that follows. The gyro is taken to have a constant bias over the run, and its scale to be right. And it needs the robot to be able to spin in place, which lesson 23-02 showed it may not on a given floor.

## What Breaks First

- **A calibration with one direction.** See `E-VEH-0023`.
- **A track width taken as constant.** See `E-VEH-0022`.
- **One spin trusted.** See `E-VEH-0024`.

## Ship It

`rc/vehicle/calibrate.hpp` holds `TurnRun`, `calibrate_effective_track`, `calibrate_scale`, `Spread` and `spread_of`.
