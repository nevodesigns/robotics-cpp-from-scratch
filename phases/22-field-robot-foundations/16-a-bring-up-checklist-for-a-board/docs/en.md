# A Bring-Up Checklist for a Real Board: Ten Cheap Checks Before the First Drive, Run Against a Fake of the Board

> The first time a new board drives a motor is not the time to find out which way is forward.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** optional: a drive board with two encoder motors, an IMU and a battery (an ESP32 or Raspberry Pi class board, about 60 to 70 dollars with motors); every test passes on a fake of the board
**Prerequisites:** 22-01, 22-06, 22-11, 21-10

## The Problem

A new board arrives, and the first thing anyone does is the thing that should be last: put it on the floor and send it a speed. What follows is a catalogue of the same afternoons. The robot spins on the spot because one motor runs the wrong way. It veers because the wheels are swapped. It goes 15 percent too far every time because the encoder counts a different number of ticks than the data sheet says. It runs into the wall at full speed because the program crashed and the board has no watchdog. Each is found in a minute **if it is looked for**, and takes an afternoon if it is found by being surprised.

A bring-up is the list of things to look for, in an order that costs least first, written down so that it is done the same way every time. This lesson writes it as a program, ten checks, and runs it against a **fake of the board**, so that it can be tested, and so that a learner without a board does the whole lesson. A learner with one runs the same function on it.

## The Concept

### The lowest level

`BoardProbe` is deliberately lower than the `Drive` of lesson 22-01. It has no speed in metres per second and no pose. It has what the hardware has: a power for each wheel, a count for each encoder, a battery voltage, an accelerometer, a gyro, and whether the emergency stop is pressed. A bring-up has to be written at this level, because its job is to find out whether the numbers that the higher level will trust are right.

### The ten checks

In the order they are made, cheapest first:

| Check | What it asks | Measured |
|---|---|---|
| `link` | does the board answer | 1 |
| `estop` | is the emergency stop released | 0 |
| `battery` | is the voltage in its range | volts |
| `imu` | does the accelerometer read gravity on z | m/s2 |
| `gyro` | does the gyro read nothing while still | rad/s |
| `direction` | do both encoders count up when driving forward | ticks |
| `sides` | does the left command move the left wheel, and only it | ticks |
| `scale` | are the ticks a metre the documented ones | ticks a metre |
| `deadband` | what is the smallest command that turns the wheels | command |
| `watchdog` | do the wheels stop by themselves | ticks |

The healthy fake passed **10 of 10**, and read **1200 ticks a metre** against the 1200 documented. Nothing moves until the first five, which cost nothing and have caught a pressed stop, a flat battery and an IMU on upside down. The `direction` check comes before `sides` and `scale`, because a reversed motor makes the other two meaningless.

### The faults, each caught by its own check

**10 faults** were put into the fake one at a time. Each failed the check meant for it and no other, except two that fail with consequences: a reversed left motor fails `sides` as well as `direction` (a command to the left wheel then makes its encoder count down) and `scale` (the two encoders now cancel), and a dead right encoder fails `scale` as well (it counts half the distance). The first line of the list is the cause, and the ones after it are what it does to the checks behind it, which is another reason to order them by what depends on what.

A tick count 15 percent off read as **1378 ticks a metre**, against 1200 expected: the board is counting at a different resolution than documented, and the advice is to check the wheel diameter, the gear ratio, the counts a turn and the decoder (x1, x2, x4). A dead band of 0.25 read as **0.25**. A person with a tape measure is what the `scale` check needs; the `DistanceMeter` is that person, and for the fake it is the fake. See `E-FIELD-0048` and `E-FIELD-0049`.

### Advice, in words

A check that fails says **what to do**. "The left encoder counts down when driving forward: swap its A and B wires, or negate it in the driver" is a line that can be acted on. "direction failed" is a line that starts a search. The advice is part of the check, and a fake that has the fault is how it was tested.

### The watchdog, tested with the cable pulled

The last check gives one command and then nothing, and requires the wheels to stop. On a real board it is the one to do with the cable pulled and a hand on the stop. A board without it was one of the faults, and the check found it. See `E-FIELD-0050`.

## Build It

`check_direction` in `exercise/solution.hpp`.

Open `exercise/solution.hpp`. The part you write is marked `TODO`; the rest of the file is already written, and it is worth reading before you start, because your part is called by it or calls it.

What to write, as the note beside the `TODO` puts it:

- Stop and note the encoders, drive both wheels forward for a second, and fail with advice if either encoder counted down or did not count.

As shipped, all 3 of the 3 tests fail. They are, in the order the suite runs them:

- A healthy board passes the ten checks, in the order that costs least first.
- Each fault fails the check of its own, and no check that is not its consequence.
- The numbers measured are the ones the fault set, and the advice says what to do about it.

```
rcpp verify 22-16
```

A failing check names the test and prints the number it expected beside the one it got, so start from the first failure and work down. When the suite is green, read `reference/solution.hpp` beside your own version: where the two differ and both pass, the difference is worth understanding.

## Use It

`rc/hal/bringup.hpp` opens nothing new in `rc::hal`.

```cpp
MyBoard board(port);          // a class that implements BoardProbe over the serial port
MyTape tape;                  // a DistanceMeter that asks the person for the number
for (const rc::hal::BringUpResult& r : rc::hal::run_bringup(board, expectations, tape))
  if (!r.ok) std::cout << r.id << ": " << r.advice << "\n";
```

**What this does not do.** The fake's faults are the ones this lesson thought of; a real board has more, and each one found on the bench is a line to add to the checklist and a fault to add to the fake. The checks drive the wheels, so they must be run with the robot on a stand or in a clear space. They check each wheel at one power; they do not check the current, the heat, a loose connector that fails when the robot is shaken, or a radio link. And passing them is the start of trust in a board, not the end.

## What Breaks First

- **A first drive with no direction check.** See `E-FIELD-0048`.
- **A tick count taken from the data sheet.** See `E-FIELD-0049`.
- **A new board with no watchdog.** See `E-FIELD-0050`.

## Ship It

`rc/hal/bringup.hpp` holds `BoardProbe`, `DistanceMeter`, `BoardFaults`, `FakeBoard`, `BringUpExpect`, `BringUpResult`, `drive_for`, `settle`, `make_result`, `check_direction`, `check_sides`, `check_scale`, `check_deadband`, `check_watchdog`, `run_bringup` and `failed_ids`.
