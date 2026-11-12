# Checking a Model Against the Simulator: A Vehicle Model as a Backend, the Experiments That Tell a Wrong Parameter From a Wrong Explanation, and the Fault They Found in Our Own Robot

> When a model and a simulator disagree, the disagreement is a measurement, and the first job is to find out which of the two it is a measurement of.

**Type:** Capstone
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 23-01, 23-07, 23-08, 22-15, 21-10

## The Problem

Lesson 22-15 compared the built in model with Gazebo and found, in one of five cases, a gap of 0.082 m and 22 degrees, and explained it. The explanation was that the wheels were scrubbing on the ground. It was a good explanation: it is what skid steer vehicles do (lesson 23-02), it fits a turn that comes out short, and it needed no further work. It was **wrong**. This lesson is about how that was found, and the procedure is the one to use every time a model and a simulator disagree: run experiments that can tell the candidate causes apart, and do not accept the first cause that would produce the symptom.

## The Concept

### The model as a backend

`ModelBackend` is a differential drive with the three things the lessons of this phase gave one: a longitudinal slip, a turn efficiency, and a lag of the wheel speeds. It sits behind the `Backend` interface of lesson 22-14, so the standard cases of 22-15 run on it exactly as they do on the built in model and on Gazebo, and `fit_efficiency` tries efficiencies and keeps the one whose worst gap to the reference traces is least. Fitted to another model of efficiency 0.8, it found **0.80**, with a worst gap of **0.0000 m**. That is what fitting is for: to find a parameter that is there.

### The question that a fit cannot answer

Fitting always returns a number. Whether the number means anything is a different question. The experiments of `turn_tests` ask it: how much of a commanded turn does the backend make **from rest**, **after two seconds of driving**, and **after driving and then a second at rest**? For a vehicle with a turn efficiency the three are the same, because the efficiency is a property of the vehicle and the floor. If they differ, the behaviour depends on what came before, and **no value of the efficiency can be right**. A model with a turn 30 percent short after it has moved (a `ContextBackend` made for the purpose) gave turn ratios of **0.97, 0.68 and 0.68**, was flagged by `context_dependent`, and could not be fitted to a worst gap below **0.013 m**, where a model without the fault fitted to **0.0000 m**. See `E-VEH-0025`.

### What it found in our own robot

Run on Gazebo, the same experiments said the following. With the weight of the robot a millimetre behind its axle (as good as over it, and as the robot file's SDF first had it), Gazebo made **1.01, 0.74 and 0.85** of the turns asked, from rest, after a drive and after a drive and a stop: right from rest and about a quarter short after a drive, and still short after a pause. That is the context dependence, and it ruled out the explanation that had been given: scrubbing wheels would have made the turn from rest short too, and changing the friction of the ground, the wheels and the caster (done as a test) changed nothing.

What does depend on whether the robot has driven is its **balance**, and how badly depends on how far the weight is behind the axle: moved 3, 6 and 10 mm back, the turn after a drive came out at **0.81, 0.87 and 0.95** of the turn asked, and at 14.4 mm (the figure in the file now) at 1.01. The fault is a gradual one, a margin of stability, and a weight exactly over the axle is the worst case and an unpredictable one. A robot with its weight over its axle rests on two wheels and a caster that touches the floor only when the body rocks, and driving and stopping rocks it. The fix was to put the weight behind the axle, 40 percent of the way to the caster (lesson 22-12), and with it Gazebo made **0.99, 1.01 and 1.02**. See `E-VEH-0026`.

With the weight right, a single efficiency fitted the five cases against Gazebo: **1.02**, with a worst gap of **0.033 m**. The model does not need a context to be right, because the simulator no longer has one. See `E-VEH-0027`.

### The procedure

1. Run the cases and read the gaps. Do not fit yet.
2. For the case with the largest gap, ask what else could cause it, and make a list.
3. For each candidate, run an experiment that **distinguishes** it from the others: change one thing and see whether the gap moves. (Friction changed: no. Time since the drive: no. Whether the robot had moved at all: yes.)
4. Only then fit, and look at whether the fit is the same across the cases.

## Build It

`turn_ratio` in `exercise/solution.hpp`.

```
rcpp verify 23-09
```

## Use It

`rc/vehicle/check_model.hpp` opens nothing new in `rc::vehicle`.

```cpp
rc::vehicle::TurnTests t = rc::vehicle::turn_tests(simulator, scenario, robot);
if (rc::vehicle::context_dependent(t)) investigate_before_fitting();
const rc::vehicle::Fit f = rc::vehicle::fit_efficiency(traces, scenario, robot, cases, slip, tau, 0.5, 1.1, 61);
```

**What this does not do.** The experiments look for one kind of context, a turn after a drive; a simulator can depend on its past in other ways (a battery, a temperature, a wheel worn in), and there is no experiment here for them. The fit is a search over one parameter on a grid, with the slip and the lag fixed. It compares traces at 0.1 s, so a difference faster than that is not seen. And a match between a model and a simulator is a statement about the two, not about the robot: that needs the robot (lesson 22-15 said so, and this one does not change it).

## What Breaks First

- **A fit made before the question of whether the model can fit.** See `E-VEH-0025`.
- **A cause accepted because it would produce the symptom.** See `E-VEH-0026`.
- **A simulator model with its weight in the wrong place.** See `E-VEH-0027`.

## Ship It

`rc/vehicle/check_model.hpp` holds `ModelBackend`, `turn_ratio`, `TurnTests`, `turn_tests`, `context_dependent`, `Fit` and `fit_efficiency`.

This ends phase 23 of the specialisation course, the models of the vehicles: a differential drive that slips, a skid steer, a car, a tracked vehicle, the grip that limits them, the motors and battery that limit them, their odometry, its calibration, and the check of a model against a simulator.
