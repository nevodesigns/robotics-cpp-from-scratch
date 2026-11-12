# The Stop That Does Not Need the App: A Latched Onboard Stop Path Driven by the Robot's Own Clock

> The most important part of the stop is the part that does not need anyone to tell it to stop.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 19-01, 14-02, 12-08, 01-06

## The Problem

The hazard that scored highest in lesson 19-01 was G2: **the robot keeps driving when the link is lost**, 16 and unacceptable. Every station written so far (phase 12) has a stop button, and a stop button is software on a screen. If the application crashes, or the network drops, or the laptop's battery dies, the button is gone and the robot is holding the last command it was given.

The answer is not a better application. It is a stop that lives on the robot, runs on the robot's own clock, and treats **silence as the signal**. The application can be wrong, hung, or dead, and the stop path does not care, because it was never waiting for the application to do anything.

## The Concept

### Silence is the signal

`StopPath` takes three things: `heartbeat(now)` from the station, `estop_line(closed)` from a physical loop, and `output(command, now)` from the robot's **own control loop**, which runs every tick whatever else is happening. `output` looks at the inputs and returns the command, or zero. Nothing in it waits for a message.

With a heartbeat every 0.1 s that stopped at 1.0 s, a timeout of 0.3 s and a loop at 100 Hz, the robot stopped at **1.3 s**, 0.3 s after the last heartbeat, with nobody sending it anything. See `E-SAFE-0004`.

The first attempt at this is a guard that checks the age of the last message **when a message arrives**. It looks right and has the one flaw that matters: with no messages, nothing ever looks. Messages stopped at 0.2 s, and the naive guard's output at 5 s was **0.8**, the last command, still driving. The stop path, called by the robot's loop, gave **0** at 5 s. A watchdog that is fed by the thing it watches is not a watchdog. See `E-SAFE-0004`.

### It starts stopped, and a missing loop is an open loop

The robot powers up **stopped**, reason `startup`, and a heartbeat alone does not change that. The e-stop loop is wired normally closed: closed is healthy, and a button pressed, a wire cut and a connector pulled all read **open**. So does a loop nobody has reported on. Heartbeats with no word from the loop read as an open loop (reason `e-stop`), and the robot stayed stopped. Safe is the default, and each thing that makes the robot move has to be earned. See `E-SAFE-0005`.

### It latches, and a reset is a decision

Once stopped, the robot stays stopped. The link came back at 2.5 s and the output was still **0**. `reset(now)` is the only way out, and it is **refused** unless the loop is closed and a heartbeat is fresh. A refused reset changes nothing: closing the loop is not a restart, a fresh heartbeat with the loop open is refused, and a closed loop with a stale heartbeat is refused. This is the lesson of the interlock in 12-08 moved onto the robot: when the reason to stop goes away, the robot must not move by itself. See `E-SAFE-0005`.

An open loop **outranks** a lost link. A robot stopped for a lost link whose loop is then opened is re-labelled `e-stop`, and closing the loop afterwards does not clear it. An open loop stops the output at the **next call**, with no timeout: a wire cut at 1.04 s gave zero on the very next call.

### Time goes only one way

The heartbeat is compared with the robot's clock. If the clock is set back, `now - last_beat` is negative, and a stale test of `> timeout` would call the heartbeat fresh for as long as the clock stays behind. A heartbeat at 100 s followed by a clock reading of 50 s gave **stale: yes** and an output of 0: a clock that goes backwards is treated as a stale heartbeat, not an immortal one. See `E-SAFE-0006`.

## Build It

`StopPath::tick` and `reset` in `exercise/solution.hpp`. The rest is given.

```
rcpp verify 19-02
```

## Use It

`rc/safety/stop_path.hpp` joins `rc::safety`.

```cpp
rc::safety::StopPath stop(0.3);
// in the robot's own control loop, every tick, on the robot's own clock:
stop.estop_line(read_loop_pin());
const double drive = stop.output(command_from_app, robot_clock_seconds());
write_motors(drive);
// in the code that receives messages from the station:
stop.heartbeat(robot_clock_seconds());
```

**What this does not do.** This is software, and a software stop is not enough on its own for a machine that can hurt someone: the control loop that calls it can hang, and then nothing calls `output`. A real vehicle puts the same idea in **hardware that does not run your code**: a watchdog timer chip that cuts the motor supply unless it is kicked, a contactor that drops out when the loop opens, a brake that is applied when it loses power. The next lesson is about those outputs. This class is the logic that those would mirror, and the thing to test before you wire the hardware. It also does not say what the timeout should be: that comes from the distance the robot travels before it stops, in a later lesson of this phase.

## What Breaks First

- **A watchdog fed by messages.** It checks only when something arrives. See `E-SAFE-0004`.
- **A robot that is allowed to move until told otherwise.** The default is to drive. See `E-SAFE-0005`.
- **A clock that goes backwards.** The stale test never fires. See `E-SAFE-0006`.

## Ship It

`rc/safety/stop_path.hpp` holds `StopReason`, `describe` and `StopPath`.
