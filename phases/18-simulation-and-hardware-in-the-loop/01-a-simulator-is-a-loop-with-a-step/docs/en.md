# A Simulator Is a Loop With a Step: Fixed Time, Variable Frames and the Spiral of Death

> A simulator that gives a different answer on a different day is not a simulator. It is a random number generator with opinions about physics.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-13, 14-12, 03-05

## The Problem

Every lesson since phase 5 has run a robot in code: a motor, a drive, an arm, a rover. Each time the caller made time go by, a step at a time, and each test said exactly how long a step was. That is the heart of a simulator, and it is easy to build wrongly the moment it is hooked to something that draws: a window, a viewer, a logger, a person pressing keys.

Such a loop runs once per frame, and a frame takes as long as it takes. A simulator that steps by however long the last frame took gives one answer on a fast machine and another on a slow one, and a third on the same machine when something else is busy. A regression test on it is a coin toss, and a bug found on one machine cannot be reproduced on another.

## The Concept

### A fixed step

The cure is to separate the **step** from the **frame**. The simulation always advances by one fixed amount, `dt`. The frame loop asks how many of them are owed:

```cpp
int advance(double elapsed);    // real seconds since the last frame; how many steps of dt to run now
```

Time that does not fill a whole step is kept for next time. A frame of 16 ms with a step of 10 ms gives 1 step, with 6 ms left over, **0.6 of a step** owed: the number `alpha()` reports, which a renderer uses to draw between the last state and the next. The next 16 ms frame gave 2 steps and the one after 1. Over 3.048 s of real time in frames of 3 ms and 16 ms, **304 steps** of 10 ms were taken and 0.8 of a step was still owed, so that no time was lost and none was invented. See `E-SIM-0001`.

A frame shorter than a step does no work at all. A negative or zero elapsed time adds nothing: a clock that stepped backwards owes the simulation nothing. A step of zero or less, which would mean a loop that never advances, is quietly replaced by a usable one. See `E-SIM-0001`.

### The same answer

A mass on a spring, simulated for 5 s with a fixed step of 5 ms, and driven by three different sets of frames:

| frames | fixed step 5 ms | step = frame time |
|---|---|---|
| steady 60 per second | 0.906213 | 0.917488 |
| steady 30 per second | 0.906213 | -0.809887 |
| jittery, 8 to 48 ms | 0.906213 | (not meaningful) |

With a fixed step all three runs took the same 1000 steps and finished at the same position to six places. Stepping by the frame time gave 0.917 at 60 frames a second and **-0.810 at 30**: not slightly different but on the other side of the spring. And the same jittery frames, run twice, gave the identical position to the last bit: 0.90621265316080712 both times. That is determinism, and it is what makes a test of a simulation worth writing. See `E-SIM-0002`.

### The spiral of death

A frame can take far too long: the window was dragged, a breakpoint was hit, the machine slept. The simulation then owes hundreds of steps, and running them takes longer than the time they stand for, so the next frame owes even more, and the loop never recovers. It is called the **spiral of death**.

The stepper allows at most `max_steps` per frame and drops the rest, counting what it drops:

```
a frame of 5 s with at most 10 steps allowed: 10 steps run, 4.90 s of real time given up; the frame after: 1 step
```

The simulation lost 4.9 s of real time and kept going, and the next frame owed one ordinary step. The count says it happened. A simulation that tries to catch up gives the person who dragged the window a frozen program for as long as the catch up takes. See `E-SIM-0003`.

### How small a step

A fixed step trades accuracy for work. The same spring, whose energy should stay at 200, with the largest error in its energy over 5 s:

| dt (s) | largest energy error |
|---|---|
| 0.050 | 200.000 |
| 0.020 | 50.000 |
| 0.010 | 22.222 |
| 0.005 | 10.526 |
| 0.001 | 2.020 |

Halving the step roughly halves the error, which is what a first order method does, and a step of 50 ms against a spring that swings in 0.31 s is 100 percent wrong. The step is chosen from what the simulated machine does fastest: a motor controller at 1 kHz needs a step of a millisecond or less. See `E-SIM-0003`.

## Build It

`advance` and `alpha` in `exercise/solution.hpp`. The constructor and the accessors are given.

```
rcpp verify 18-01
```

The frames come from a generator seeded in the test, so the figures above are the same on every run.

## Use It

`rc/sim/stepper.hpp` joins `rc::sim`.

```cpp
rc::sim::FixedStepper stepper(0.001, 50);
// every frame:
const int n = stepper.advance(seconds_since_last_frame);
for (int i = 0; i < n; ++i) world.step(0.001);
draw(world, stepper.alpha());
```

**What this does not do.** It chooses no step: the right dt is a property of the plant, and lesson 14-12 is where one comes from. It does not interpolate: `alpha()` is the number a drawing routine would use, and a state to blend with is the caller's. It takes the real time it is given and does not read a clock, so a fast forward or slow motion is a matter of scaling what is passed in. And it makes a simulation reproducible only if everything the simulation reads is too: a random
number or a thread race inside the world is a different lesson.

## What Breaks First

- **A step equal to the frame time.** A different answer on every machine. See `E-SIM-0002`.
- **Leftover time thrown away.** A simulation that runs slow by the fraction a frame does not fill. See `E-SIM-0001`.
- **Catching up with everything.** A frozen program, then a worse one. See `E-SIM-0003`.

## Ship It

`rc/sim/stepper.hpp` holds `FixedStepper`.

The simulation now runs on a clock of its own, which every later lesson of this phase takes for granted.
