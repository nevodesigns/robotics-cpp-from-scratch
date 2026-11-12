# Prove It Still Works: A Self-Test That Runs Every Check, a Permit That Expires, and the Stop That Cannot Be Tested From Software Alone

> A safety function that has never been seen to work is a safety function you hope works.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 19-10, 19-03, 19-02, 01-06

## The Problem

Everything in this phase so far is **dormant**. The stop path does nothing for months, until the one time it is needed. The brake coil is released and engaged once a day. The e-stop loop is closed every minute of the robot's working life and opened, for real, perhaps never. A dormant function can fail **without anybody knowing**: a relay that has welded, a loop with a cut wire behind a connector, a sensor that stuck, a watchdog that was disabled during debugging and left that way. The failure is not noticed because nothing it does is visible, and it is found out by the accident it was fitted to prevent.

That is called a **latent fault**, and the only defence against it is to **test the function on purpose**, before each use and at regular intervals.

## The Concept

### Run every check, and let nothing hide

`SelfTest` is a list of named checks, each a function that returns true or false. `run` runs **all** of them, in order, and reports each. Five checks: two passed and three failed, one by returning false, one by **throwing** and one because the function **was not there**. The failed names were `e-stop loop reads, battery sensor, fence loaded`, all three, after a first failure that a short-circuit would have reported alone. A check that throws is a failure and not a crash: a sensor that does not reply is exactly what the test is for. And a check with an empty function is a failure too, so that a check that was never written fails, instead of vanishing from the list. See `E-SAFE-0031`.

### Nothing checked is not a pass

An empty list is **not** a pass: `all_passed` of nothing is false. Two passing checks passed. A self-test that someone has emptied, or whose checks failed to register, would otherwise be the cleanest result on the robot. See `E-SAFE-0031`.

### A permit that runs out

`Permit` is the robot's permission to drive: it is given only after a test that passed, and it lasts an **interval**. With an interval of 3600 s a test at 100 s that passed permitted driving up to **3700 s** exactly and not at 3700.5 s. It was not given before any test. It was not given if the clock had gone backwards. And a test that **failed** at 2000 s cancelled the earlier pass, even though that was an hour fresh: a failed test is a stronger fact than an old pass. A pass at 2100 s gave it back. The robot that has to be re-tested before it drives is a robot that cannot have been left with a failure from last week. See `E-SAFE-0032`.

### A test that tests the software tests the software

The stop that matters is the one that cuts the motor supply. Its path is: the loop opens, the stop logic commands the contactor open, the contactor opens. A contactor can **weld**: its contacts stick closed after switching a heavy load, and then commanding it open does nothing.

Two checks. One asks the software: does the stop logic command the supply open when the loop is open? The other **tests the stop**: command the contactor open and read its auxiliary contact. With the contactor welded closed, the software check **passed**, and the check that reads the contactor **failed**. With a healthy one, both passed. The software was right in both cases, and in one of them the robot could not stop. A test that is only a read of its own answer is a test of the code, and this lesson is about the **machine**: a safety check has to exercise the real path, to the last thing that can be read back. See `E-SAFE-0032`.

### How often

A function that fails at a constant rate has failed by time `t` with a probability of `1 - exp(-rate * t)`. With one dangerous failure in 100000 hours (1e-5 per hour, about eleven years):

| time since the test | chance it has failed |
|---|---|
| 1 hour | 1.00e-05 |
| 1 day | 2.40e-04 |
| 1 week | 1.68e-03 |
| 1 month | 7.27e-03 |
| 1 year | 8.39e-02 |

A year without a test is an 8 percent chance of a dead stop function. Turned around, `max_interval` gives the longest interval for a target: to keep the chance at 1 in 1000 the test is due every **100.1 hours** (4.2 days), and at a rate of 1e-6 every **1001 hours**. That is the real content of "test it regularly": a rate, a target and a number of hours. The test interval is not a matter of taste. See `E-SAFE-0033`.

## Build It

`SelfTest::run`, `all_passed`, `Permit::allowed`, `failure_probability` and `max_interval` in `exercise/solution.hpp`.

```
rcpp verify 19-11
```

## Use It

`rc/safety/selftest.hpp` joins `rc::safety`.

```cpp
rc::safety::SelfTest test;
test.add("e-stop loop opens the contactor", [&] { return stop_test_with_real_contactor(); });
const auto results = test.run();
permit.record(robot_clock(), rc::safety::all_passed(results));
if (!permit.allowed(robot_clock())) refuse_to_drive(rc::safety::failed_names(results));
```

**What this does not do.** It runs checks and keeps a clock. It cannot make a check **meaningful**: whether the check exercises the real path is a decision of the person who writes it. A test can also be unsafe to run (opening a loop while the robot is moving), and the self-test must run only when the robot is stationary and the supply is not live to the wheels. The failure rate of 1e-5 is a number from the data sheet or the field, and a constant rate is an assumption that is wrong for parts that wear out. A test that is run on every start up and passed does not show that the function works **at the moment it is needed**, only that it did a moment ago: the interval is what limits that, and the answer for a high risk is **diagnostics that run all the time** (the heartbeat of lesson 19-02 is one) and not tests that run now and then. And a test that cannot fail has not tested anything: break the thing deliberately and see that it is caught (lesson 18-06 does this to a controller).

## What Breaks First

- **A short-circuited test report, an empty test that passes.** See `E-SAFE-0031`.
- **A permit that never runs out, or a test of the software only.** See `E-SAFE-0032`.
- **A test interval chosen by habit.** See `E-SAFE-0033`.

## Ship It

`rc/safety/selftest.hpp` holds `CheckResult`, `SelfTest`, `all_passed`, `failed_names`, `Permit`, `failure_probability` and `max_interval`.
