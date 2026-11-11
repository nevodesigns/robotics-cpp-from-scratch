# The Bench Link: A Controller and a Plant Emulator Exchanging Frames, and What Each Does When the Other Goes Quiet

> On the bench, the motor is real and the controller is a program, and the thing between them is a cable that has views about reliability.

**Type:** Build
**Time:** about 130 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 18-02, 18-04, 17-05

## The Problem

A **hardware in the loop** rig puts a real piece of the robot, or a faithful emulator of it, at the end of a link, and the controller program somewhere else. It is the step between simulation and the machine: the controller is the real code, running on its real schedule, and the plant is a model that answers over the same kind of link the hardware will use. Everything the link does that a function call does not, it now does: it delays, it loses frames,
it delivers them out of order, it goes quiet.

Two questions decide whether the rig is safe and whether it is honest. What does the plant side do when the controller stops talking? And what does the controller side do with a measurement that arrives later than a newer one? This lesson builds both ends and measures each.

## The Concept

### A frame says which it is

Commands go one way and measurements the other, and each carries a number. A **command** has a sequence number. A **measurement** carries the emulator's **tick**, which command it was made with, and the position. Without the numbers a frame that arrives late cannot be told from the present.

### The emulator

The plant side acts on the newest command and holds it between commands, like a motor driver. A command **older** than the newest already seen, one that was overtaken on the way, is ignored and counted:

```
commands 2 (value 3) then 1 (value -9) arriving in that order: the plant used command 2, position 5.520, 1 ignored
```

Command 1 was sent before command 2 and arrived after it. Acted on, the motor would have turned round for one step and the controller's most recent decision would have been undone by a stale one. See `E-SIM-0025`.

### When the controller goes quiet

A held command is the right behaviour for a gap of a tick or two and the wrong behaviour for a gap of a second. A motor that is told to run and then hears nothing keeps running. One command of 6 followed by a second of silence:

| plant side | position after 1 s | speed |
|---|---|---|
| holds the last command | 10.920 | 12.000 |
| watchdog of 5 ticks | 0.720 | 0.000 |

The plant that held the command was at full speed, 12, and had travelled ten times as far. The **watchdog** is the plant's own rule: if no new command has come for more than 5 ticks, it acts on zero. It spent 94 ticks on the watchdog in that second, and a new command wakes it again. A watchdog of zero means none, which is a choice, and on a real bench the wrong one. See `E-SIM-0026`.

### What the controller does with what comes back

Measurements arrive late and out of order. The controller side keeps the newest **by the emulator's tick** and refuses any that is not newer: measurements with ticks 5, 7, 6, 7 gave 2 kept and 2 refused (one overtaken, one repeated). At the end of each tick, a tick on which nothing new came is counted as **held**: the controller carried on with the last measurement, and two quiet ticks were counted 2. See `E-SIM-0027`.

### The loop over the link

A position loop to 1, 3 s at 100 Hz, with a delay of one tick and increasing loss in each direction:

| loss each way | error in the last half second | measurements held | commands ignored |
|---|---|---|---|
| 0.00 | 0.0001 | 1 | 0 |
| 0.05 | 0.0001 | 19 | 0 |
| 0.10 | 0.0001 | 34 | 0 |
| 0.20 | 0.0002 | 64 | 0 |
| 0.40 | 0.0007 | 115 | 0 |

The loop settles at every loss, since each command is built from the latest measurement and a lost one is replaced by the next. At 40 percent, 115 ticks of the 300 were spent on a measurement that was not new, and the error was still under a thousandth. A controller that is this tolerant of loss is the reason a lossy link does not need the reliable delivery of lesson 17-05 for a stream like this. A command stream also does not need it: the next command supersedes the last.
See `E-SIM-0027`.

### The link dies

The target steps to -1 at 0.95 s and the link dies from 1.0 s to 2.0 s in both directions. The position of the plant when the link came back:

| plant side | position | ticks on the watchdog |
|---|---|---|
| holds the last command | **-21.85** | 0 |
| watchdog of 5 ticks | -1.45 | 95 |

The last command was the strongest the controller could give, and the plant that held it ran for a second at its top speed to -21.85, twenty times the distance anyone wanted. The one with the watchdog stopped in 5 ticks and coasted to -1.45. This is the test to write first for any bench: not the loop that works, but what the plant does when the other end disappears. See `E-SIM-0026`.

### Frames that overtake each other

With a delay of 1 to 4 ticks per frame, frames overtake: **69** measurements arrived older than one already seen, and the controller side refused all 69. The emulator ignored 69 overtaken commands too. A controller that took each measurement as it arrived would have acted on an old one 69 times. The effect on this loop was small: error over the whole run 0.1604 with the check and 0.1611 without, and in the last half second the naive one was a hair better, 0.0126 against
0.0130. The cost shows in the age of the information: on average the measurement acted on was 2.17 ticks old with the check and 2.42 without. A velocity worked out from successive measurements, or an estimator, would see the difference more than a proportional gain does. See `E-SIM-0027`.

## Build It

`on_command`, `step` of `PlantEmulator` and `accept`, `end_tick` of `ControllerSide` in `exercise/solution.hpp`. The frames, the constructors and the accessors are given.

```
rcpp verify 18-09
```

The link in the tests loses and delays frames by generators seeded in the test, so the figures above are the same on every run.

## Use It

`rc/sim/bench_link.hpp` joins `rc::sim`.

```cpp
rc::sim::PlantEmulator emulator(2.0, 0.1, 12.0, 5);
rc::sim::ControllerSide controller;
// plant side:      emulator.on_command(frame);   send(emulator.step(dt));
// controller side: if (controller.accept(frame)) ...;   controller.end_tick();
```

**What this does not do.** The link in the tests is a model: a real serial line or network has bursts of loss and a delay that depends on load. The emulator is exactly the SimMotor of lesson 18-02 and has no bugs of its own to find. It does not authenticate frames or check them for damage (lesson 15-13's CRC does). The watchdog sets the command to zero, which is the safe state for a motor and not for every plant: an arm
holding a load needs to hold, and a vehicle on a slope needs to brake. And it counts ticks, so it assumes the emulator and the controller agree on how long a tick is.

## What Breaks First

- **A plant that holds its last command for ever.** A motor running on a controller that has gone away. See `E-SIM-0026`.
- **A command or a measurement taken as the present because it arrived last.** Acting on old information. See `E-SIM-0025`.
- **A link with no number on its frames.** Nothing to tell stale from new. See `E-SIM-0027`.

## Ship It

`rc/sim/bench_link.hpp` holds `CommandFrame`, `MeasureFrame`, `PlantEmulator` and `ControllerSide`.

A bench can now run the real controller against an emulated plant, and say what each does when the other falls silent.
