# The Contract Test for a Hardware Interface: Eleven Promises Every Drive Must Keep, Checked on a Fake, a Simulator and a Board Alike

> An interface says what can be called; a contract says what happens, and it is the contract that a fake, a simulator and a board must all pass.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 22-01, 22-03, 21-10

## The Problem

Lesson 22-01 wrote `Drive` and a fake of it, and said that a program written against the one would run on the other and on a board. That is a claim about what happens, and a class with six functions makes no promise about what happens. Whether `command` limits, whether `stop` stops, whether a base that is no longer heard from stops itself: these are not in the types. They are in the **contract**, and a contract that is only in somebody's head is kept by whichever implementation that person wrote last.

The cure is a program that is the contract: one function that takes any way of making a `Drive` and tries it on every promise. It is run on the fake, on the simulator's drive, and (in a hardware section, with a fake of the device when no board is to hand) on a board. If they pass the same function, the program above them is safe to move between them. If one does not, the failing line is in words.

## The Concept

### Eleven promises

`check_drive_contract` makes a **fresh** drive for each promise, gives it the commands that promise needs, steps it, and records what was not kept as `tag: what happened`:

| Tag | The promise |
|---|---|
| `rest` | it starts at rest, with a state that is a number |
| `forward` | a positive speed moves it forward, and straight commands go straight |
| `turn` | a positive turn rate turns it to the left |
| `limit` | it never goes faster than its limit |
| `stop` | after `stop()` it has stopped within the time given |
| `watchdog` | with no commands it stops itself |
| `nan` | a command that is not a number does not move it or spoil it |
| `odometry` | its pose agrees with its own speed |
| `repeatable` | the same commands give the same pose |
| `step` | a step ten times longer gives the same place |
| `zero` | a step of no time changes nothing |

What the drive is *expected* to be (its limits, how long a stop may take, its watchdog) is passed in as `DriveExpect`, so the same function judges a toy base and a heavy one. With a limit of 0.5 asked of a drive that can do 1.0, the `limit` promise was broken, and with a stop allowed only 0.3 s, so was `stop`.

### The fake keeps them all, and six broken drives do not

The fake of lesson 22-01 broke **0** promises. Each of six drives that break one thing was caught by the promise it breaks:

| Drive | Promises broken |
|---|---|
| no watchdog | **1** (`watchdog`) |
| no limit | **2** (`limit`, and a `stop` that takes too long from that speed) |
| turns the wrong way | **1** (`turn`) |
| odometry 10 percent long | **1** (`odometry`) |
| `stop()` does nothing | **1** (`stop`) |
| the clamp as first written | **2** (`watchdog` and `nan`) |

### The bug it found in the fake

The last row is not invented. It is the clamp `std::max(-limit, std::min(limit, x))`, and the fake of lesson 22-01 had it. `std::min(limit, NaN)` returns the limit, because `NaN < limit` is false, so **a command that is not a number became full speed**. The same was true of the joint bus, where a NaN target made a joint run away at its maximum rate. Neither was seen by the tests of 22-01. The `nan` promise found both, in the first run, and they are fixed there (a command that is not a number is a stop, and a joint bus refuses it as a whole). That is the use of a contract: it is written by someone who has thought of what can go wrong, and it is run by everyone. See `E-FIELD-0033`.

### Joints

`check_joint_contract` does the same for a row of joints, for any class that has the six functions: wrong sized or NaN commands must be **refused and change nothing**, a joint must never move faster than its rate, it must **reach** its target, and it must **hold** it. Three broken buses were caught: one that jumps to the target at once (`rate`), one that accepts any command (`size` and `nan`), one that creeps off the target by a hundredth of a millimetre a step (`hold`, and it also failed the check that a refused command changes nothing, because it crept while idle: **2** broken, against **1** for the jumping bus and **3** for the sloppy one). See `E-FIELD-0034`.

## Build It

`check_drive_contract` in `exercise/solution.hpp`.

```
rcpp verify 22-11
```

## Use It

`rc/hal/contract.hpp` opens nothing new in `rc::hal`.

```cpp
const auto broken = rc::hal::check_drive_contract(
    []() { return std::unique_ptr<rc::hal::Drive>(new MyBoardDrive(port)); }, expectations);
for (const std::string& what : broken) report(what);
```

**What this does not do.** It checks promises about a drive moving in the plane for seconds; it cannot see a thermal limit, a brownout, or the first second after power-up. Its tolerances (5 percent on the odometry, a hundredth of a metre between step sizes) are set for the fakes of this course and will need to be set for a board from the board's own data. It tests one drive at a time, so it cannot find a fault that only shows when two are on one bus. And a pass is the absence of a failure it knows to look for: the point of the broken drives is that it was shown to look.

## What Breaks First

- **A clamp that turns NaN into the limit.** See `E-FIELD-0033`.
- **An interface with no contract.** See `E-FIELD-0034`.
- **A watchdog that nobody tested.** See `E-FIELD-0035`.

## Ship It

`rc/hal/contract.hpp` holds `DriveExpect`, `DriveFactory`, `drive_run`, `state_is_finite`, `check_drive_contract` and `check_joint_contract`.
