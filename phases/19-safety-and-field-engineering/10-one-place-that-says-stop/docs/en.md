# One Place That Says Stop: A Supervisor That Combines Every Monitor, Latches, Keeps Every Cause, and Never Gets Less Strict When a Fault Is Added

> Seven monitors that each work are not a safety system. A safety system is one place that decides, whatever they say.

**Type:** Build
**Time:** about 130 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 19-09, 19-08, 19-07, 19-06, 19-05, 19-02

## The Problem

Lessons 19-02 to 19-09 each built one monitor: a stop path, an output policy, a speed from the range, a fence, a sensor cross-check, a power monitor and a tilt monitor. Each says something different, in a different type, at its own rate, and each has an opinion on what the robot may do.

If the control code reads them one by one, each combination of them is a place for a mistake. Does a fence warning plus a derated battery mean the lower of the two speeds, or the last one read? What if one of them says stop and another says go? What stops the robot from moving again by itself when the one that said stop clears while the other is still in trouble? That is the supervisor: **one function** from every monitor's state to one answer, and the answer is built to be **safe in the cases nobody thought of**.

## The Concept

### Nothing known is a stop

`Inputs` has a field for each monitor, and every default is the **unsafe** one: the loop open, the heartbeat stale, the fence position unknown, a tilt stop, a faulted pair, a power stop, and a range cap of 0. An `Inputs` that nobody filled in gave **stopped**, a cap of 0, and **all six causes**. A forgotten field is therefore a stop and not a robot that goes on, and the supervisor itself starts stopped: it cannot move until a `reset` has seen a healthy set of inputs. This is the same rule as lesson 19-02, now for the whole machine. See `E-SAFE-0028`.

### Stop, or slow, and by how much

A **stop cause** is one of six: the loop open, the heartbeat stale, the fence outside or unknown, a tilt stop, a faulted sensor pair, the power monitor's stop. Any of them is a stop, and the cap is 0. A **limit cause** is a reason to go slower: a fence or tilt warning or a suspect pair (the slow speed, 0.5 m/s), a derated battery (the top speed times the power share), a range that allows less than the top speed (the range's cap, and 0 if that is not a number). When several apply, the **lowest** cap wins. A tilt warning and a power share of 0.2 gave **0.4 m/s**, the lowest of 0.5 and 0.4, and both reasons were reported (tilt 2 plus power 8 is 10 in the limit bits). The cap of a derated power was 0.8 m/s at a share of 0.4, and a range of 0.3 m/s capped the robot at 0.3.

### Latching, and every cause kept

Once any stop cause is present, the supervisor latches. A robot stopped for a tilt, whose link was then lost as well, had **both causes on record**: `link, tilt`. When both went away it was **still stopped**, and it stayed stopped until a `reset`, which was **refused** while a cause remained (and changed nothing when it was refused) and accepted once all were clear. After the reset the mode was normal and the causes `none`.

Keeping **every** cause matters for two reasons. The cause that appears first is not always the cause that matters: a robot that stopped for a lost link and was also tilting needs somebody to look at the tilt, and a log that said only "link lost" would send them to the router. And a reset after one cause has gone, with another one still there, is exactly the way a robot starts to move into a hazard that the operator had not been told about. See `E-SAFE-0029`.

### The property: adding a fault never makes it safer

Ten conditions, each present or not (the loop open, the heartbeat stale, a fence warning, a fence outside, a tilt warning, a tilt stop, a suspect pair, a faulted pair, a derated power, a power stop), make **1024** combinations. For every one, a stop cause present gave stopped and a cap of 0, and for every one and every one of the 10 conditions added to it, the mode and the cap were **no less strict** (`normal < degraded < stopped`). The count:

| answer | combinations |
|---|---|
| stopped | 1008 |
| degraded | 15 |
| normal | 1 |

**0 violations** in 10240 additions of a fault. Only **one** combination in 1024, the all clear, lets the robot go at full speed. That is the shape of a safety function: not a list of cases each of which was thought about, but a property that was checked over **all** of them. A rule such as "a tilt warning cancels a fence warning" would have passed any single test somebody wrote, and failed the table. See `E-SAFE-0030`.

### Words for the log

`causes_text` writes the bits as `fence, tilt, power`, or `none`. A log that says `cause 44` has to be decoded at 3 am.

## Build It

`stop_causes`, `limit_causes`, `speed_cap`, `Supervisor::step` and `reset` in `exercise/solution.hpp`. The types and `causes_text` are given.

```
rcpp verify 19-10
```

## Use It

`rc/safety/supervisor.hpp` joins `rc::safety`.

```cpp
rc::safety::Supervisor supervisor{rc::safety::SupervisorConfig()};
// every tick, in the robot's own loop (lesson 19-02):
const rc::safety::Decision d = supervisor.step(inputs);
const double drive = std::min(std::fabs(command), d.speed_cap) * (command < 0 ? -1.0 : 1.0);
if (d.mode == rc::safety::Mode::stopped) log(rc::safety::causes_text(d.stop_causes));
```

**What this does not do.** It is a function of the inputs and a latch, and it is only as good as the monitors that fill them: a monitor that says "inside" when it is not makes the supervisor wrong in the same way. It assumes that all the monitors are evaluated in the same loop on the same tick, which is a design choice that must be kept: a supervisor that reads a monitor whose answer is two seconds old is back to lesson 19-05's stale range. It does not replace the independent stop path of lesson 19-02, which is the part that works if the process that holds the supervisor dies: **the supervisor decides, and the stop path and the hardware enforce**. And the speed cap is a cap, not a speed: the thing that follows it has to brake no faster than the robot can (lesson 19-04).

## What Breaks First

- **A forgotten input that defaults to "fine".** See `E-SAFE-0028`.
- **A stop that clears when its cause does, or a reset that forgets the second cause.** See `E-SAFE-0029`.
- **A rule that was tested for the cases someone thought of.** See `E-SAFE-0030`.

## Ship It

`rc/safety/supervisor.hpp` holds `Inputs`, `SupervisorConfig`, `Mode`, `StopCause`, `LimitCause`, `Decision`, `stop_causes`, `limit_causes`, `speed_cap`, `Supervisor` and `causes_text`.
