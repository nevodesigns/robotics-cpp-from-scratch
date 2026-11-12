# Two Simulators, One Scenario, and the Gap Between Them Measured, Explained and Shrunk by Calibrating the Built In Model to the Other

> Two simulators that disagree are not a problem, they are a measurement, and a measurement can be used.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-10, 22-14, 21-10

## The Problem

Lesson 22-14 ran one program on two simulators and they ended 0.24 m and 23 degrees apart. That is a fact about two simulators and it can be read two ways. One is "they disagree, so something is wrong". The other is "they disagree by an amount that can be measured, and a number can be assigned to the cause." The second is more useful, and it needs a procedure: a way to compare two runs, a standard set of things to compare them on, and a way to find out what to change.

## The Concept

### The gap between two traces

`compare_traces` takes two traces sampled at the same times and returns, sample against sample, the **largest** and the **root mean square** distance between the poses, and the distance and the heading difference at the **last** sample. Two traces that are the same gave zero; one shifted by 0.1 m gave 0.1 for all three, and a heading turned by a quarter gave 90 degrees. Both measures matter: the end can be right while the middle was wrong, and a gap that is largest in the middle is the sign of a difference in how the robot gets there, which is a different difference from where it arrives. See `E-FIELD-0045`.

### Four cases, each from rest

A single run says little, so `standard_cases` is five: **straight**, **turn** in place, **arc**, **stop**, and **drive, turn, drive**. They are the things a base is asked to do, each starting from rest, and each shows a different difference: the straight drive shows the lag in speeding up, the turn the lag in the turning, the arc both, the stop the way the speed runs out, and the last whether a turn that follows a drive is the same as one from rest. `run_cases` runs each on both backends, started afresh for each, and `gap_table` writes the gaps in rows. Starting afresh matters: a run that begins where the last ended compares the end of one case with the start of the next.

### Reading a time constant off a trace

The fake drive has a time constant, a setting. Gazebo has a wheel that is driven by a joint controller, and its time constant is nothing anyone wrote down. But it **can be read** from a run: start from rest at a steady speed command, take the speed in each sample from the distance moved (`speeds_of`), take 63.2 percent of the final speed, and find when the speed first reached it. `response_time_constant` does this, interpolating between samples and counting each speed at the middle of its interval, because a speed measured from two poses is an average. On first order responses made with time constants of 0.05, 0.20 and 0.35 s, it read them back to within 0.012 s. See `E-FIELD-0046`.

### Without Gazebo: the loop, between two built in models

Where Gazebo is not installed the whole procedure can still be shown. Two built in models, one with a time constant of 0.05 s and one of 0.30 s, differed by **0.050 m** at the worst over the five cases. The step response of the slower, read by the function above, gave **0.299 s**. The faster model, given that time constant, differed by **0.0001 m**. Measure, read the cause, set it, measure again: that is calibration, and the check is that the gap went down.

### Between the built in model and Gazebo

The five cases were run on both, each started afresh (Gazebo is reset, not restarted), with the built in model's time constant at 0.05 s, a round number. The worst gap was **0.081 m**: **0.004 m** on the straight, **0.005 m** on the turn and **0.005 m** on the stop. Then the step response of Gazebo's robot was read with the same function as for the synthetic one: a time constant of **0.040 s**. The built in model was given that, and the worst gap was **0.082 m** (straight 0.002, turn 0.005, stop 0.005). The first four cases were within a few millimetres to begin with, because 0.05 s was already close to 0.04 s, so the new constant changed them by almost nothing: that is what calibrating a model that was nearly right looks like, and a result that had to be reported as it came out.

The last case is the one that stays: **drive, turn, drive** was **0.081 m** and **22.8 degrees** apart before calibration and **0.082 m** and **22.8 degrees** after. A turn from rest is the same in the two simulators; a turn that follows a drive is not, because in Gazebo the wheels scrub on the ground as the robot turns, stick and slip, and turn it less than was asked, which the built in model has nothing to say about. One time constant cannot close that.

What remains is what the two simulate differently and one time constant does not capture. It is a gap that is now known and small, and not one that was there and unseen.

### What a gap tells you

A gap that falls when one parameter is set is a difference of **that parameter**. A gap that does not is a difference of something else: a friction, a contact, a different integration step. The procedure finds the first kind and exposes the second. It does not say which simulator is right. For that there is only the robot, which is why the course keeps a hardware section for every track. See `E-FIELD-0047`.

## Build It

`compare_traces` and `response_time_constant` in `exercise/solution.hpp`.

```
rcpp verify 22-15
```

## Use It

`rc/field/gap.hpp` opens the module `rc::field`.

```cpp
rc::field::BuiltinBackend builtin(0.05);
rc::gz::GzBackend gazebo;   // where Gazebo is installed
const auto cases = rc::field::standard_cases();
const auto gaps = rc::field::run_cases(builtin, gazebo, scenario, robot, cases, 0.1);
std::cout << rc::field::gap_table(cases, gaps);
```

**What this does not do.** It compares two simulations of a robot on flat ground at low speed, with no terrain, walls or contact in either; the cases that bring those in belong to the tracks that need them. A time constant is the 63 percent time of one run: a response that is not first order (a delay, an overshoot, a rate limit) is not captured by it. Sampling at 0.1 s for the cases and 0.01 s for the step response is a choice, and a gap smaller than the sample interval's distance is not seen. And it assumes the two are given the same commands at the same simulated times, which is why Gazebo is stepped and not left to run.

## What Breaks First

- **A comparison made at the end only.** See `E-FIELD-0045`.
- **A model setting taken from a data sheet, not a run.** See `E-FIELD-0046`.
- **A gap read as one simulator being wrong.** See `E-FIELD-0047`.

## Ship It

`rc/field/gap.hpp` holds `TraceGap`, `compare_traces`, `speeds_of`, `response_time_constant`, `GapCase`, `standard_cases`, `run_cases`, `worst_gap` and `gap_table`.
