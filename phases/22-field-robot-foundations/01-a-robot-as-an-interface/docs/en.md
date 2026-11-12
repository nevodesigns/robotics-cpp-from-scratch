# A Robot as an Interface: Drive, ArmBus and LegBus, and a Fake That Behaves Like the Hardware

> Everything above the hardware talks to a class with six functions, and the same program then runs on a simulator, a fake, or a board.

**Type:** Build
**Time:** about 75 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 21-10, 18-02, 00-05

## The Problem

The courses that follow put a program on a car, an arm and a dog. The program should not know which. The way to make that true is a **boundary**: a small class that is everything the program may ask of the hardware, written first, so that the program is written against the class and the hardware is written to it. Then the same program runs against a simulator today, a **fake** on a laptop tomorrow, and a board next month, and the only thing that changes is which object is made.

This lesson writes the boundary for the three kinds of robot of the course: `Drive` for a base, `ArmBus` for an arm's joints, `LegBus` for a quadruped's. It also writes a **fake of the hardware**, `FakeDrive`, because a class with no implementation cannot be tested, and a fake that is too kind teaches a program habits that the real thing punishes.

## The Concept

### What a drive is

Six things. `command` says what is wanted: a forward speed in m/s and a turn rate in rad/s. `stop` says that nothing is. `step(dt)` lets time pass, because a fake has to be moved and a board has its own clock (its `step` does nothing). `state` reports what the base is actually doing. That is all, and every real base can be made to fit it, which is the test of an interface: how many things have to be hidden behind it.

### What a believable fake does

A fake that does exactly what it is told in no time is the kindest possible lie. `FakeDrive` does three things that the hardware does:

1. **It limits, and it is not fooled.** A command that is not a number is treated as a stop; the first version of this fake let `std::min` turn it into the limit, which is full speed (found by the contract test of a later lesson). Told to go 5 m/s, a base with a limit of 1 m/s did **1.000 m/s** after 10 s.
2. **It lags.** The motor and the controller inside the base answer in a time constant, here 0.2 s: the speed reached 63.2 percent of a step in **0.200 s**. A controller written against a base that answers instantly is tuned for a robot that does not exist.
3. **It watches the clock.** A base whose commander has crashed must not keep driving. After 0.5 s with no command, `FakeDrive` stops itself: a base told 0.5 m/s once was below 0.01 m/s after **1.265 s** (0.5 s of waiting, and then the lag of the slowdown). See `E-FIELD-0001`.

A stop is not instant either: a base at 1 m/s told to stop **coasted 0.200 m**, which is the speed times the time constant. See `E-FIELD-0002`.

### Arms and legs

An arm and a set of legs are, to the boundary, the same thing: a row of joints given target angles that they reach at no more than a rate. `JointBus` does that, and refuses a command with the wrong number of joints instead of half applying it. A command with a NaN in it is refused too, because `std::min` and `std::max` would turn it into the largest step and the joint would run away. A three joint arm told to move 1 rad at 0.5 rad/s took **2.000 s**; twelve leg joints at 2 rad/s took **0.500 s**. `ArmBus` and `LegBus` are separate types so that a program cannot give an arm's targets to a leg. See `E-FIELD-0003`.

## Build It

`FakeDrive::step` in `exercise/solution.hpp`: age the command, apply the watchdog, lag the speeds towards the target, then move the pose.

```
rcpp verify 22-01
```

## Use It

`rc/hal/interfaces.hpp` opens the module `rc::hal`.

```cpp
rc::hal::FakeDrive base;
rc::hal::DriveCommand go;
go.v = 0.5;
base.command(go);
base.step(0.02);
```

A program takes a `rc::hal::Drive&` and never learns which one it got.

**What this does not do.** The fake is a first order lag with a limit: it has no acceleration limit, no slip, no battery sag, no encoder noise and no latency on the link. Those are the lessons that follow. The interface has no way to ask what the base is capable of, or whether it is healthy; the contract test later in this phase adds the second.

## What Breaks First

- **A base with no watchdog.** See `E-FIELD-0001`.
- **A stop that is taken to be instant.** See `E-FIELD-0002`.
- **An arm's targets given to a leg.** See `E-FIELD-0003`.

## Ship It

`rc/hal/interfaces.hpp` holds `DriveCommand`, `DriveState`, `Drive`, `DriveLimits`, `FakeDrive`, `JointBus`, `ArmBus` and `LegBus`.
