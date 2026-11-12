# How Likely After the Fix: Fault Trees, the Average Chance a Safety Function Is Dead, and Why Common Cause Limits Redundancy

> "We added a second one" is not a number. This lesson makes it one, and shows how little a second one is worth when they share a cause.

**Type:** Build
**Time:** about 130 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 19-12, 19-11, 19-01, 03-03

## The Problem

Lesson 19-01 scored G2, a robot that keeps driving when the link is lost, at 16: unacceptable. Lessons 19-02 to 19-12 built what is meant to answer it. The risk assessment of the last lesson has to say **how much lower** the risk is after them, and "much lower" is the kind of sentence that a reviewer takes apart. The likelihood in the matrix was a guess. After the mitigation it has to be a **calculation**, from numbers that can be argued with: how often the thing is demanded, how likely the safety function is to be dead when it is, and how much a second channel really helps.

## The Concept

### Probabilities that are honest about what is not known

`p_and` multiplies independent probabilities (all of them have to happen) and `p_or` is one minus the chance that none does (any of them will do). `p_and({0.1, 0.2})` is 0.02 and `p_or({0.1, 0.2})` is 0.28. Two rules make them safe. A probability that is **not a number counts as 1**: `p_and({0.001, NaN})` is 0.001, which says that a protection whose chance of failing is unknown has been counted as failed. And the empty cases are the cautious ones: an `and` of nothing is 1 (it needs nothing to happen), an `or` of nothing is 0. Values are brought into 0 to 1. See `E-SAFE-0037`.

### A fault tree, and its single points

A **fault tree** is the top event (what you do not want) as a tree: a basic event has a probability, an `all_of` node happens when all its children do, an `any_of` node when any one does. `probability` evaluates it; `single_points` names the basic events that **on their own** make the top event happen, the ones reached through `or` nodes only.

Take "the robot moves after the e-stop loop opens". Either the contactor is welded (1e-4), or the software stop fails (1e-3) **and** the brake fails (1e-2). Top event **1.100e-04** per demand, and **one single point: contactor welded**. Nearly all of the 1.1e-4 is that one event, since the other branch is 1e-5. Put a second contactor in series, so that **both** have to be welded: the top event fell to **1.001e-05** and there were **0** single points. A single point is where the effort goes, and it is found by reading the tree and not by averaging. An `all_of` with a single child protects nothing and passes that child's single point up. See `E-SAFE-0038`.

### The chance a tested function is dead

Lesson 19-11 gave the chance that a function has failed by the end of the test interval. What matters for a demand that can come **at any time** is the **average** over the interval, `pfd_avg`: for a rate of 1e-5 per hour,

| test every | exact | rate times interval over 2 |
|---|---|---|
| 1 day | 1.200e-04 | 1.200e-04 |
| 1 week | 8.395e-04 | 8.400e-04 |
| 1 month | 3.641e-03 | 3.650e-03 |
| 1 year | 4.258e-02 | 4.383e-02 |
| 10 years | 3.340e-01 | 4.383e-01 |

The common shortcut, half of rate times interval, is good while the product is small and **overstates** it when it is not: 31 percent too high at ten years. The yearly figure is half of the 8.39e-02 of lesson 19-11, which is the chance at the **end** of the year; on average over the year the function is dead half as often. See `E-SAFE-0038`.

### Two channels, and what they share

Two channels, either of which is enough, each with the same pfd: if they fail independently the pair fails with the **product**. But some failures take both: a shared power supply, a shared wire, one design error copied into both, one mistake by the technician who maintained both. A fraction **beta** of the failures are of that kind, and the pair's pfd is `((1 - beta) pfd)^2 + beta pfd`. With a single channel tested monthly at 3.64e-03:

| common cause share, beta | two channels | better than one by |
|---|---|---|
| 0.00 | 1.33e-05 | 275 times |
| 0.01 | 4.94e-05 | 74 times |
| 0.05 | 1.94e-04 | 19 times |
| 0.10 | 3.75e-04 | 10 times |
| 0.20 | 7.37e-04 | 5 times |

With **10 percent** common cause a second channel is **10 times** better, not 275: the term `beta * pfd` is nearly all of the answer, and the part with the square is lost in it. At beta of 1 there is no gain at all. This is why lesson 19-07 said the second sensor should differ in how it measures, in its power and in its wiring, and why lesson 19-03 put the brake on a spring and the cut-off on a different supply: **the number to improve is beta**, and a better channel does nothing for it. See `E-SAFE-0039`.

### From a number back to the matrix

The demands on a safety function come at some rate, and the hazard happens when a demand meets a dead function: `hazard_frequency = demands per year times pfd`. `likelihood_from_frequency` maps it onto the scale of lesson 19-01: 5 at once a year or more, 4 from once in ten years, 3 from once in a hundred, 2 from once in a thousand, 1 below. G2, with **20 link losses a year** and a severity of 4:

| design | pfd | per year | likelihood | score | class |
|---|---|---|---|---|---|
| software stop, tested yearly | 4.26e-02 | 8.52e-01 | 4 | 16 | unacceptable |
| software stop, tested monthly | 3.64e-03 | 7.28e-02 | 3 | 12 | high |
| plus a hardware cut-off, beta 0.10 | 3.75e-04 | 7.50e-03 | 2 | 8 | medium |
| plus a hardware cut-off, beta 0.02 | 8.56e-05 | 1.71e-03 | 2 | 8 | medium |
| plus a hardware cut-off, beta 0.00 | 1.33e-05 | 2.65e-04 | 1 | 4 | **medium** |

Testing monthly and not yearly moved it a whole class. A hardware cut-off took it to medium. Cutting beta from 0.10 to 0.02, a fivefold improvement, **did not change the class**. And the last row, an ideal pair with no common cause, scores **4 by the product, which reads low**, and is **medium** all the same, because of the floor of lesson 19-01: a severity of 4 is never classed lower. That is the right result. No amount of arithmetic makes a robot that can kill somebody a low risk, and **the residual risk of a catastrophe is something to be accepted by a named person, in writing**, and not computed away. See `E-SAFE-0039`.

## Build It

`clamp_probability`, `p_and`, `p_or`, `probability`, `single_points`, `pfd_avg`, `pfd_1oo2`, `hazard_frequency` and `likelihood_from_frequency` in `exercise/solution.hpp`. `Node` and its builders are given.

```
rcpp verify 19-13
```

## Use It

`rc/safety/risk_math.hpp` joins `rc::safety`.

```cpp
const double pfd = rc::safety::pfd_1oo2(rc::safety::pfd_avg(1e-5, 730.0), 0.10);
const int likelihood = rc::safety::likelihood_from_frequency(rc::safety::hazard_frequency(20.0, pfd));
const rc::safety::RiskClass after = rc::safety::risk_class(4, likelihood);
```

**What this does not do.** Every number in it is an input and not a finding. The failure rates are from data sheets, which are optimistic, or from field data, which a new robot does not have; the demand rate is a guess until the robot has run; and beta is a judgement, which standards put at between 1 and 10 percent for well separated designs, and is the number to be most suspicious of. The tree takes events as **independent**, which is false for anything that shares a cause, and a tree with a hidden common cause is the usual way to arrive at an answer that is a thousand times too good. It handles one top event at a time and does not find **cut sets**, the minimal combinations that cause it (a tool such as a spreadsheet of a fault tree is not a substitute for thinking about how the machine fails). It uses constant failure rates, which is wrong for parts that wear. The scale of likelihood is the simplified one of lesson 19-01, and **a real assessment uses the scale in the standard that applies and the safety integrity level it sets for the function**.

## What Breaks First

- **A probability nobody knew, counted as small.** See `E-SAFE-0037`.
- **A single point of failure in a design with a second channel.** See `E-SAFE-0038`.
- **Redundancy that shares its cause.** See `E-SAFE-0039`.

## Ship It

`rc/safety/risk_math.hpp` holds `clamp_probability`, `p_and`, `p_or`, `Node`, `basic`, `all_of`, `any_of`, `probability`, `single_points`, `pfd_avg`, `pfd_1oo2`, `hazard_frequency` and `likelihood_from_frequency`.
