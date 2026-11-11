# Pass or Fail: Running a Scenario and Judging Every Step of the Trace Against Written Criteria

> A test that is passed by looking at it has not been written yet.

**Type:** Build
**Time:** about 130 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 18-05, 18-06, 18-08, 14-04

## The Problem

The scenario of lesson 18-05 tells a story to the simulation: go to 1, then at 2.5 s to -0.5, then at 5 s back to 1. The simulation runs it and leaves a trace. Someone looks at the trace, says "that looks fine" and moves on, and next month a change makes it a little worse and nobody looks. A scenario is only a test if the run ends in **pass or fail**, and if the failure says what failed.

Lesson 14-04 measured a step response with four numbers: rise time, overshoot, settling time and final error. A scenario has several steps, in both directions, and each one needs the four. This lesson does the measuring over a whole run, and turns the numbers into a verdict.

## The Concept

### What a step is

A step is one change of target, and it lasts until the next change or the end of the trace. For each, the position just before it is where it **started**, and the new target is where it is **going**. The four numbers:

- **overshoot**: how far past the new target the position went, as a **fraction of the size of the step**;
- **rise time**: from 10 to 90 percent of the way;
- **settling time**: from the change until the position stayed within a **band** of the target for the rest of the step, or -1 if it never did;
- **final error**: the new target minus the position at the end of the step.

For a first order lag of 0.2 s stepped from 0 to 1, the numbers should be known: no overshoot, a rise time of tau ln 9, which is 0.4394 s, and a settling time into 2 percent of tau ln 50, 0.7824 s. The measured numbers were 0.0000, 0.4400 and 0.7900 s, within a sample of 10 ms of both, and the final error was nothing. A measurement that agrees with the maths is one that can be trusted on the traces whose answers are not known. See `E-SIM-0031`.

### Size, and sign

The overshoot is a fraction of the step so that a big step and a small one can be compared, and the settling band is a fraction of the step for the same reason: 2 percent of a step of 1.5 is 0.03, and of a step of 0.1 is 0.002.

A step **down** is the same thing with the arrow reversed. Past the target means below it, and a measurement that takes "past" to be "above" would see a downward overshoot as an undershoot and report none. An underdamped step up from 0 to 1 and its exact mirror, down from 0.5 to -0.5, gave identical numbers:

| direction | size | overshoot | rise (s) | settling (s) |
|---|---|---|---|---|
| up | 1.00 | 0.0759 | 0.1800 | 0.5600 |
| down | -1.00 | 0.0759 | 0.1800 | 0.5600 |

See `E-SIM-0031`.

### What is not a step

A change of target to where the position already is moves nothing, and has no overshoot, rise or settling to speak of, so it is left out, rather than reported as a step of perfect numbers. A change at a time beyond the end of the trace is left out. Four changes, one of no size and one beyond the end, gave **2** steps measured, at 0 and 4 s. And each step stops at the next change: the first step's numbers come only from its own two seconds.
Where a run never stays inside the band, as an oscillation about the target does not, the settling time is **-1**, which is not a time, and the verdict says "settling" and not "settled in -1 seconds". See `E-SIM-0032`.

### The verdict

`verdict` takes the measured steps and the criteria, an overshoot no more than a fraction, a settling no more than a time, and a final error no more than a limit, and gives one reason for each step and each criterion that was broken, **in step order**:

```
step at 2.5 s: overshoot; step at 5.0 s: settling; step at 5.0 s: final error
```

An empty verdict is a pass. A test fails on a non empty one and prints the list, which says which step and what about it, so that the person looking at it knows where to look before they open a plot. See `E-SIM-0033`.

### A scenario, judged

The scenario of lesson 18-05 with three targets, run through the simulation with the position loop of lesson 18-02:

| step at (s) | from | to | overshoot | rise (s) | settling (s) | final error |
|---|---|---|---|---|---|---|
| 0.0 | 0.02 | 1.00 | 0.253 | 0.110 | 0.640 | -0.0000 |
| 2.5 | 1.00 | -0.50 | 0.249 | 0.110 | 0.640 | 0.0000 |
| 5.0 | -0.50 | 1.00 | 0.249 | 0.110 | 0.640 | -0.0000 |

The three steps are the same to three places, which is what a linear loop should do: a step down is the mirror of a step up. Against criteria of an overshoot of 0.3, a settling of 1.0 s and a final error of 0.02, the verdict was **none**: a pass. Against the same criteria:

- with a sensor **60 ms late** (lesson 18-04) every step failed overshoot, settling and final error: the loop never settled inside the band;
- with the sensor **stuck from 1 s** (lesson 18-06) the first two steps failed, the first on settling and final error and the second on all three, and the last on settling and final error only: its overshoot was nothing, because the position never moved at all.

The same scenario file, three simulated plants, three different verdicts, and each says what it was about. See `E-SIM-0033`.

## Build It

`analyse_steps` and `verdict` in `exercise/solution.hpp`. The trace, the step metrics and the criteria are given.

```
rcpp verify 18-11
```

Everything is deterministic, so the figures above are the same on every run.

## Use It

`rc/sim/judge.hpp` joins `rc::sim`.

```cpp
const auto steps = rc::sim::analyse_steps(trace, change_times, 0.02);
const auto failures = rc::sim::verdict(steps, rc::sim::Criteria{0.3, 1.0, 0.02});
for (const std::string& f : failures) std::cout << f << "\n";
```

**What this does not do.** It measures steps of the target and no other kind of test: a disturbance that pushes the plant off a steady target has a recovery time that this does not measure. It uses one band for settling, where a real specification may have two. It has no notion of the command: a controller that reaches the target by slamming the motor against its limit passes, and lesson 18-02's peak speed
is where that shows. And it trusts the change times it is given: a trace and a scenario that were not run together give a verdict about nothing.

## What Breaks First

- **A test passed by eye.** The slow worsening nobody sees. See `E-SIM-0033`.
- **"Past" taken to mean "above".** Downward overshoot reported as none. See `E-SIM-0031`.
- **A step of no size counted.** Perfect numbers that mean nothing. See `E-SIM-0032`.
- **A settling time of -1 reported as a time.** A pass for a loop that never settled. See `E-SIM-0032`.

## Ship It

`rc/sim/judge.hpp` holds `Trace`, `StepMetrics`, `analyse_steps`, `Criteria` and `verdict`.

A scenario can now end in pass or fail, and the failure names its step.
