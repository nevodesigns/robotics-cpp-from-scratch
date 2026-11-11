# One Seed, Many Streams: Randomness That Does Not Change When You Add a Sensor

> A bug that appears when you add an unrelated sensor to the simulation is not a bug in the sensor. It is a bug in how the dice were shared out.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 18-04, 03-03, 01-10, 01-05

## The Problem

Lesson 18-04 gave each simulated sensor a seed so that it was repeatable. A simulation has many parts that use randomness: the encoder's noise, the IMU's noise, the dropouts of a link, a push on the plant. If each is given its own number as a seed by hand, someone has to keep the list, and two parts end up with the same seed and the same noise. If all of them draw from **one** generator, as is natural, then each part's numbers depend on how many
draws the others made before it.

That second arrangement is the trap. Run a simulation to find a bug, add a logging camera to see more, and the imu's noise changes, because the camera drew some numbers first. The bug moves or disappears. A simulation in which adding an observer changes the outcome is not reproducible in the only way that matters.

## The Concept

### Mixing

The cure begins with a **mixer**: a function that turns any 64 bit number into one that looks unrelated to it. Seeds that are 1, 2 and 3 should not give streams that begin the same way. `mix64` is one step of splitmix64: add a large odd constant, then two rounds of xor with a shifted copy and a multiplication. The test changed one bit of the input and counted how many of the 64 output bits changed:

```
on average 32.1 of 64 output bits differ
```

Half. Which is what an unrelated number would give, and is called the avalanche property. A mixer that flipped only a few bits would give seeds 1 and 2 streams that look alike. See `E-SIM-0019`.

### A seed from a name

Each part of the simulation is given a **name**, and its seed is made from the master seed and the name:

```cpp
mix64(mix64(master) ^ fnv1a(name))
```

The same master and the same name always give the same seed. Neither depends on what other parts exist or the order they were made in. Names that differ by one letter, `motor0` and `motor1`, gave seeds that differed in **32.1 of 64 bits** on average, over 200 pairs: unrelated. Changing the master seed or the name changes the seed; an empty name is allowed. See `E-SIM-0019`.

### A stream

A `Stream` is a source of numbers seeded this way. `uniform()` gives 53 bits, from 0 up to but not including 1, and `gaussian()` sums twelve of them and subtracts six. 100 000 numbers:

| | mean | spread |
|---|---|---|
| uniform | 0.4992 | variance 0.0830 (1/12 is 0.0833) |
| normal | 0.0001 | 0.9999 |

A seed of zero, which would leave a xorshift generator stuck for ever, is not stuck here: successive numbers differ. See `E-SIM-0020`.

### Independent, and repeatable

`Streams` hands out a stream by name, makes it the first time it is asked for, and returns the same one after. The encoder's and the imu's streams from one master seed had a correlation of **-0.0040**: no relationship. The encoder's stream in a second run of the same seed was identical to the first, to the last number, and a different master seed gave different numbers.
Asked for a second time, a stream carries on where it was and does not start again. And the **order** in which streams were first asked for made no difference: in two simulations that asked for them in opposite orders, the motor's first two numbers (0.912040 and 0.609638) and the imu's (0.190753) were identical. See `E-SIM-0020`.

### What adding a part does

Three sensors each read noise on every step for 1000 steps, and then a camera is added to the middle of the list. The imu's noise, before and after:

| arrangement | imu values that changed |
|---|---|
| a stream each | **0 of 1000** |
| one shared stream | **1000 of 1000** |

With a stream per name the camera does not exist as far as the imu is concerned. With one shared stream every number the imu gets is a different one, from the first step on: the imu has not been given different noise by the camera, it has been given the camera's numbers. The bug that was reproducible at 3 a.m. is gone at 9 a.m. after someone added a logger, and nothing about the logger is to blame. See `E-SIM-0021`.

## Build It

`mix64`, `derive_seed`, `Stream::next` and `Streams::get` in `exercise/solution.hpp`. `uniform`, `gaussian` and `count` are given.

```
rcpp verify 18-07
```

Nothing is timed. Every number comes from a seed, so the figures above are the same on every run.

## Use It

`rc/sim/streams.hpp` joins `rc::sim`.

```cpp
rc::sim::Streams streams(master_seed);
const double encoder_noise = 0.002 * streams.get("encoder").gaussian();
const double imu_noise = 0.01 * streams.get("imu").gaussian();
```

**What this does not do.** It is not a source of secure random numbers and must not be used as one. The gaussian is an approximation with no values beyond six standard deviations, which is right for sensor noise and wrong for a study of rare events. A stream per name is only as good as the names: two parts that use the same name share a stream, and a name built from something that changes between runs, a pointer or a time, throws reproducibility away. And streams are independent in the sense
that they do not repeat each other, which is a statistical statement, not a proof.

## What Breaks First

- **Seeds that are neighbouring numbers.** Streams that begin alike. See `E-SIM-0019`.
- **A stream's numbers that depend on the order of asking.** A different run when the code is refactored. See `E-SIM-0020`.
- **One generator shared by the whole simulation.** Adding an observer changes the result. See `E-SIM-0021`.

## Ship It

`rc/sim/streams.hpp` holds `mix64`, `derive_seed`, `Stream` and `Streams`.

A simulation can now be extended with new parts without disturbing the dice of the old ones.
