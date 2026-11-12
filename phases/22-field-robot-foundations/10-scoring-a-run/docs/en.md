# Scoring a Run: Navigation, Manipulation and Gait Metrics, and Criteria That Turn Numbers Into a Pass or a Fail

> Reached the goal is a fact about the last sample; how it got there is the score.

**Type:** Build
**Time:** about 80 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-06, 22-07, 22-08, 21-10

## The Problem

"It reached the goal" is a statement about the last sample of a run. It is silent about the wall it went through on the way, the five minutes it took, the slow trial in twenty, and the stride it missed. A test that says only pass or fail on the end of a run lets every one of those through. This lesson writes the scoring for the three kinds of robot of the course, so that a run is a **set of numbers with a threshold on each**, and a failure says which.

## The Concept

### Navigation

`score_navigation` reads the `pose` channel of a log against its scenario. It counts the goals reached **in the order asked** (a robot that reaches the second goal first has not done the first), the time of the last, the length of the path, and its **straightness**: the straight distance from the first pose to the last over the length walked. A straight 10 m run scored **1.0000**; a V shaped detour of 2 m off the line at the middle scored **0.9285**, because it walked 10.77 m to go 10. The clearance is the distance to the nearest wall or post less the robot's radius, so a **negative clearance is a collision** and its size says how deep. See `E-FIELD-0029`.

### Collisions are events, not samples

A robot driven straight through two posts spent **110 samples** inside them. That is two mistakes: `score_navigation` counts a collision when the clearance first goes below zero and not again until it has come back, and so counted **2** for the two posts. A count of samples depends on the rate the log was written at, so it is not a score, it is an artefact. See `E-FIELD-0030`.

### Manipulation

A pick and place is tried many times, and the numbers that matter are the **rate of success**, the **time** and the **error**. `summarise_trials` gave **90.0 percent** for 20 trials with two failures, a mean of **4.72 s** and a 95th percentile of **9.00 s**. The two are far apart because one trial was slow, and a cell that must keep a cycle time cares about the second. Failed trials do not enter the times: counting a failure as zero seconds makes a bad cell look fast. See `E-FIELD-0031`.

### Gait

A legged robot's score is in its rhythm. From one foot's contact signal, `gait_metrics` gives the **duty factor** (the fraction of the time on the ground, **0.60** for a walk with 30 of 50 samples down), the **stride period** (**0.500 s**) and the **irregularity**, the spread of the periods over their mean: **0.000** for a steady gait and **0.222** with one missed step. See `E-FIELD-0032`.

### Criteria

`failures()` compares a navigation score with a set of thresholds and returns, in words, what it does not meet: "reached 0 goals of 1", "straightness 0.9285 is below 0.95". An empty list is a pass. The words matter: a failed test that says only "false" costs a person the time to find out why.

## Build It

`score_navigation` in `exercise/solution.hpp`.

```
rcpp verify 22-10
```

## Use It

`rc/field/score.hpp` opens the module `rc::field`.

```cpp
rc::field::Criteria need;
need.max_time = 60.0;
need.min_clearance = 0.3;
const rc::field::NavScore s = rc::field::score_navigation(scenario, log, 0.25);
for (const std::string& why : rc::field::failures(s, need)) report(why);
```

**What this does not do.** The robot is a disc for the clearance, which is wrong for a long vehicle or an arm. Collisions are against the walls and posts of the scenario only. Straightness compares the end with the start and not with the best path, which a planner would know. The manipulation numbers are given, not measured from a log, and the gait from one foot, not four.

## What Breaks First

- **A pass judged on the last sample.** See `E-FIELD-0029`.
- **Collisions counted in samples.** See `E-FIELD-0030`.
- **A mean time that hides the slow trials.** See `E-FIELD-0031`.
- **A gait judged by its average.** See `E-FIELD-0032`.

## Ship It

`rc/field/score.hpp` holds `NavScore`, `distance_to_segment`, `clearance_at`, `score_navigation`, `Trial`, `TrialSummary`, `summarise_trials`, `GaitMetrics`, `gait_metrics`, `Criteria` and `failures`.
