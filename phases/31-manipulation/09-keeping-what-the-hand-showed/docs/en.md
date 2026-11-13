# Keeping What the Hand Showed: Recording a Demonstration, Resampling It to a Steady Rate, Trimming the Idle Ends, and Checking It Is Worth Keeping

> A demonstration is only as good as its timestamps, and most of the bad ones can be found by looking at them before anything learns from them.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 31-08, 31-02, 18-07

## The Problem

31-08 let a person lead the follower arm. What a person did can be **kept**, as a **demonstration**: the leader's joints (the **action** the person gave), the follower's joints (what was **observed**) and the hand's opening, with a **time** on each. A later course can learn from demonstrations (the plan says so plainly: **learning is not in these courses**), and the part that belongs here, in plain C++, is the **record**: what to store, and how to tell a good one from a bad one **before** anything is trained on it.

## The Concept

### An episode of samples

A **sample** is a moment: time, leader's six joints, follower's six, the hand. An **episode** is the list from the start of one task to its end. Messages never arrive at a steady rate: an episode that was recorded at a nominal **30 Hz** for **5.0 s** had **151 samples** and a longest gap of **0.039 s**, because of jitter in the arrival times. See `E-ARM-0100`.

### A steady rate by interpolation

What learns from or replays an episode wants **equal steps**. **Resampling** interpolates linearly onto a grid; the episode above gave **151 samples**, each exactly one step from the last, following the original to within a hundredth of a radian. Linear interpolation is right for the **joints**; it is not right for anything that jumps (a gripper command that is open or closed, or an image), which should be taken from the **nearest** sample.

### Trimming the idle ends

A person starts the recording, then reaches for the part; and stops, then ends it. The **idle ends** are not the task, and they teach "do nothing", which is the wrong lesson. **Trimming** finds the first and the last sample at which the leader's speed was over a threshold and keeps those and a little **padding** before and after. Trimming **1 s** of idling at each end left **2.8 s**: a little under the three seconds of the reach, because the first and last moments of a smooth reach are slower than the threshold (a lower threshold or more padding keeps them). An episode that never moves is **empty** after trimming. See `E-ARM-0101`.

### Action and observation do not line up

The follower is **behind** the leader (31-08's lag). The delay can be found: shift the follower's joint back by the amount that makes it agree with the leader best. It was found at **100 ms**, where it was **100 ms** (to within a sample). That delay is what has to be allowed for when the action is **paired** with the observation that follows it, and it is a number to record with the data.

### Is it worth keeping

An episode is checked for the **longest gap** (a dropped stretch of frames makes the motion in it a guess), its **duration**, and the **fraction idle**. One that lost **20** frames in a row had a longest gap of **0.70 s** and was **refused** (**0**), where the good one was kept (**1**). Bad demonstrations are cheaper to find at the time than after a model has learned from them. See `E-ARM-0102`.

## Build It

`trim_idle` in `exercise/solution.hpp`.

```
rcpp verify 31-09
```

## Use It

`rc/arm/demo_record.hpp`.

```cpp
rc::arm::Episode e = rc::arm::trim_idle(rc::arm::resample(recorded, 30.0), 0.02, 3);
if (!rc::arm::check_episode(e, 0.02, 0.2, 2.0, 0.3).ok) discard();
```

**What this does not do.** It stores **joints and a hand**, and **no images**: a camera (31-03) adds a frame to each sample, with a time of its own that has to be matched to the others (the lag lesson, again), and files of a size this lesson does not touch. It does not write to **disk** or choose a **format**. It does not say whether a demonstration was **good as a way to do the task** (a clumsy one that succeeded passes every check here), which only an outcome check (31-02's machine ending in done) or a person can say. **No learning** is here at all; the clean command interface (follower joints, hand) is what a later course would sit on.

## What Breaks First

- **Samples taken to be evenly spaced.** See `E-ARM-0100`.
- **Idle ends left on the episode.** See `E-ARM-0101`.
- **A demonstration with a hole in it.** See `E-ARM-0102`.

## Ship It

`rc/arm/demo_record.hpp` holds `Sample`, `Episode`, `max_gap`, `resample`, `leader_speed`, `trim_idle`, `estimate_lag` and `check_episode`. With it phase 31 is complete: an arm that picks a thing up and puts it down, finds the thing with a camera whose place it has measured, feels when it has touched, inserts with give, keeps its speed to what a person near it can bear, and can be led by hand, with the demonstrations kept.
