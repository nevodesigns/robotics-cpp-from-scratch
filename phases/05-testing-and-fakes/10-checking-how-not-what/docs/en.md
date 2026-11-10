# Checking How and Not What: A Test That Recorded the Calls Instead of Looking at the Motor

> A fake can write down what it was asked to do, or it can behave like the thing it stands for. A test of the first
> breaks when the program is improved. A test of the second breaks when the program is wrong.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 03-05, 03-14, 05-02

## The Problem

A program brings a motor up to speed in four steps. Its test hands it a fake motor port that writes down every call, and checks that the calls were `enable`, `set 25`, `set 50`, `set 75`, `set 100`. It passes.

Next month somebody ramps in eight steps for a smoother start. The motor behaves better. The test fails. The expected list is edited. Then somebody removes a second, redundant `enable`. The test fails again.

Lesson 05-02 made a fake sensor. This asks what a fake for something you send commands to should check.

## The Concept

### Two fakes

A **recording** fake writes down the calls. It makes it easy to assert what was done: that is the list.

A **simulating** fake behaves like the thing. `SimPort` keeps the state of a motor, whether it is enabled and how fast it turns, and it has the rules the real driver has. A speed set while the driver is disabled is ignored by the hardware, and a speed past the limit is refused. The fake counts each as a **violation**, so a test can ask "did the program ever break a rule, and where did the motor end up?" without caring which calls were made.

### What the fake does

Set before enabling: 1 violation, speed unchanged. A speed of 1.5 against a limit of 1.0: 2 violations, speed unchanged. A speed of -1.0, exactly at the limit and going backwards: allowed, not counted. A disabled motor is not turning, so `disable` sets the speed to zero. See `E-TEST-0025`.

### The cost of recording

Five changes that do not matter: a ramp in 8 steps, a second `enable`, the final speed set again, a `disable` first, and a ramp in 2 steps.

| change | test of the calls | test of the state |
|---|---|---|
| ramp in 8 steps instead of 4 | fails | passes |
| enable twice | fails | passes |
| set the final speed again | fails | passes |
| disable before starting | fails | passes |
| ramp in 2 steps | fails | passes |

**5 of 5** broke the test of the calls, and **0 of 5** broke the test of the state. See `E-TEST-0024`.

### Both catch bugs, one says what they did

Three bugs: never enabling the driver, a last step past the target and the limit, and ending a step short. Both kinds of test caught all **3**. A list of calls can only say that two lists differ. The state says what happened:

- never enabling: **4** speeds ignored by the driver, final speed 0;
- last step too big: **1** speed refused, final speed **0.75**.

A failure that names the consequence is one a person can act on. See `E-TEST-0026`.

## Build It

`SimPort::enable`, `set_speed` and `disable` in `exercise/solution.hpp`. The `MotorPort` interface and the accessors are given.

```
rcpp verify 05-10
```

## Use It

`rc/test/sim_motor.hpp` joins `rc::test`.

```cpp
SimPort port(1.0);
bring_up(port, 1.0);
RC_CHECK(port.enabled());
RC_CHECK_NEAR(port.speed(), 1.0, 1e-12);
RC_CHECK_EQ(port.violations(), 0);
```

**What this does not do.** A call sequence is sometimes the requirement: a safety interlock that must disable before it moves, or a protocol where order is the contract. Then a recording test is right, and the point of this lesson is to choose it on purpose and not by default. The fake has no timing, so it cannot show a motor that was asked to change speed too quickly, and it does not model what
the motor does in response (lesson 05-08 simulated a plant). And a fake with the wrong rules is worse than none: it must be checked against the real device once.

## What Breaks First

- **A test of the calls.** A harmless change fails it. See `E-TEST-0024`.
- **A fake with no rules.** The hardware's refusals go unseen. See `E-TEST-0025`.
- **A failure that names no consequence.** Slow to read. See `E-TEST-0026`.

## Ship It

`rc/test/sim_motor.hpp` holds `MotorPort` and `SimPort`.

Lesson 05-08 said to assert a behaviour and not a number. This says to check the state a sequence leads to and not the sequence.
