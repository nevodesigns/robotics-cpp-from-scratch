# The Test That Broke When Nothing Was Wrong: Asserting a Behaviour, Not a Number

> The suite goes red after a change that made the robot no better and no worse. The expected number was
> written down from the first run, and it records an accident.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 05-03, 05-04

## The Problem

A controller is working. Its test runs the simulation and checks that the response at one second is 0.933636, a number copied from the run that was good. A month later somebody halves the time step, or corrects the plant's time constant by five percent, and the test fails. Nothing has got worse. The expected
number is updated to the new value, and the test now guards the arithmetic of one particular run and nothing else.

Lesson 05-03 had a test catch a bug, and lesson 05-04 found inputs nobody thought of. This is about what a test should say in the first place, for something that moves.

## The Concept

### A response, in numbers

A step response is the output after the target jumps. Three numbers describe what a person asks of it:

- **overshoot**: how far past the target it goes, as a percent of the target;
- **settling time**: the last moment it was more than 2 percent from the target;
- **steady error**: how far from the target it ends, as the mean of the last tenth of the run so one noisy sample does not decide it.

A fourth fact, **diverged**, says it ran away: a value that is huge or not a number. It has to be its own flag, because a value that is not a number fails every comparison quietly and a bug can hide there. `measure` reads all four from a response, and `meets` checks them against a `Spec`.

### The pinned value

The suite pinned the value at one second, 0.933636, and then made six changes that should not matter:

| change | value at one second | exact test |
|---|---|---|
| time step halved | 0.933610 | fails |
| time step doubled | 0.933686 | fails |
| plant 5 percent slower | 0.936651 | fails |
| plant 5 percent faster | 0.930499 | fails |
| plant 10 percent slower | 0.939517 | fails |
| plant 10 percent faster | 0.927268 | fails |

**6 of 6** broke it. See `E-TEST-0018`.

### The behaviour

The same six changes, and five real bugs, against a specification of at most 10 percent overshoot, 3 seconds to settle and a steady error of at most 0.01:

| variant | overshoot % | settling s | steady error | verdict |
|---|---|---|---|---|
| the six harmless changes | 0.00 | 1.69 to 2.02 | 0.0000 | meets |
| integral term lost | 0.00 | 10.00 | 0.3333 | fails |
| gains too low | 0.00 | 10.00 | 0.3780 | fails |
| integral too strong | 33.56 | 2.43 | 0.0000 | fails |
| gains too high | 18.29 | 0.45 | 0.0000 | fails |
| sign flipped | 0.00 | 10.00 | runs away | fails |

All 6 harmless changes pass and all 5 bugs are caught. See `E-TEST-0019`.

### Not too tight

A specification can be made so tight that it is the pinned value again. With the settling time cut to 1.8 s, **4 of 6** harmless changes failed it: the base design settles in 1.865 s. A bound should come from what the application needs, and a margin is what lets harmless changes through. See `E-TEST-0020`.

## Build It

`measure` and `meets` in `exercise/solution.hpp`. The plant and controller, `simulate`, and the `Spec` are given.

```
rcpp verify 05-08
```

## Use It

`rc/test/response.hpp` joins `rc::test`.

```cpp
const Response r = simulate(kp, ki, tau, dt, 10.0);      // or a recorded response from the robot
RC_CHECK(meets(Spec(), measure(r)));
```

**What this does not do.** It is for a step response of one output. A robot with several joints or a response to a disturbance needs its own list of numbers. It does not say how to choose the specification: that comes from the application, and a number too loose lets bugs through as surely as one too tight breaks the build. And it
does not replace an exact test where the answer really is exact: a conversion or a parser should be pinned to the digit.

## What Breaks First

- **A pinned value.** A harmless change fails it. See `E-TEST-0018`.
- **A behaviour test that checks one number.** A bug in another gets through. See `E-TEST-0019`.
- **A bound copied from the last run.** The pinned value again. See `E-TEST-0020`.

## Ship It

`rc/test/response.hpp` holds `Response`, `simulate`, `Metrics`, `measure`, `Spec` and `meets`.

Phase 05 ends in a suite you can prove catches bugs. This is what makes it one that does not also cry wolf.
