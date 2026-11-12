# What the Output Does When Nothing Is in Charge: Safe Values per Channel and a Brake That Applies on Loss of Power

> A stop signal that is sent is only half of a stop. The other half is what each output does when it can no longer be told anything.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 19-02, 19-01, 03-03, 01-06

## The Problem

Lesson 19-02 decides **when** to stop. It does that in software, and the software can fail in ways that leave no code running at all: a hang, a reset, a brown-out, a crashed process on a board with no operating system to clean up after it. At that moment nothing is in charge, and every output on the robot does something. What it does is not chosen by the control code, which is gone. It is chosen by the **output stage** and by **how the hardware was wired**, and if nobody decided, the default decides.

The default is almost always **hold**: the output stage keeps the last value it was given. A motor driver holding 80 percent is the exact opposite of a stop.

## The Concept

### Every channel has a safe value, and a policy to reach it

A `Channel` has a name, a `safe_value` (what it must be at for the machine to be safe) and a `FailPolicy` for what the output stage does when the controller dies: `hold`, `zero`, or `safe_value`. `unsafe_on_failure` lists the channels that would end up somewhere else.

Five channels (the two drives, steering, a brake coil and a horn), all left on the default, with the controller dead: unsafe were **drive_left, drive_right, steering and brake_coil**. The horn is not listed because it was at 0 when the controller died: holding a safe value is safe. Set all five to zero and the list was **empty**. The list is the point: before this lesson the question "what does the robot do when the controller dies" had no answer that could be read out of the code. See `E-SAFE-0007`.

### Zero is not always safe

A zero is safe where zero means stopped. A servo speed controller (an ESC) is neutral at a pulse of 1500 microseconds, and a pulse of **0** is not a stop, it is no signal, and what a controller does with no signal is up to its manufacturer: some stop, some hold, some brake hard. An ESC pulse channel on the zero policy was **unsafe**; given its own safe value it went to **1500** and the list was empty. The same goes for an arm joint (zero torque lets it fall), a steering servo (centre, not zero) and a gripper (hold, because opening drops what it carries). The safe value is a decision about the machine, written down per channel. See `E-SAFE-0008`.

### Choose the hardware so that failure is the stop

Some failures cannot be handled in software at all, because the software is not there. A **spring applied** brake is held on by a spring and released by a coil at 12 V or more. A **power applied** brake is the other way round: the coil applies it. The robot is moving in each case, so the first has 24 V in the coil and the second has nothing. Then:

| the robot is moving, then | spring applied | power applied |
|---|---|---|
| supply lost | engaged | **released** |
| wire cut | engaged | **released** |
| controller dead, output stage drives zero | engaged | **released** |
| controller dead, output stage holds its last command | **released** | **released** |

A spring applied brake was engaged in **3 of 4** and a power applied one in **0 of 4**. With a power applied brake, **every** way the brake can fail is a way of not stopping, and the brake works only while everything around it is working. With a spring, the loss of power, a break in the wire and a dead controller **are** the command to stop. This is the idea of **energise to run**: the machine moves only while something is actively holding it permitted, and the loss of anything is the removal of that permission. See `E-SAFE-0009`.

### The case the spring does not cover

The fourth row: the controller dies and the output stage **holds** 24 V on the coil, and the brake stays released. The spring does nothing about that, because the coil is being driven. The brake coil is a channel, and its safe value is 0 V. On the `hold` policy the coil stayed at 24 V and the brake was released. On the `safe_value` policy the coil went to **0 V** and the brake was engaged: 4 of 4 with the two ideas together. Neither is enough alone, and the list from `unsafe_on_failure` is how to tell. See `E-SAFE-0009`.

## Build It

`output_on_failure`, `unsafe_on_failure` and `brake_engaged` in `exercise/solution.hpp`.

```
rcpp verify 19-03
```

## Use It

`rc/safety/failsafe.hpp` joins `rc::safety`.

```cpp
std::vector<rc::safety::Channel> channels = {{"drive_left", 0.0, rc::safety::FailPolicy::zero}};
const auto unsafe = rc::safety::unsafe_on_failure(channels, last_commands);
if (!unsafe.empty()) report_unsafe_design(unsafe);
```

Keep the table of channels next to the wiring diagram, and run `unsafe_on_failure` in a test, with last commands taken from a fast moving scenario.

**What this does not do.** It models three policies, and a real output stage has others (a pin that goes high impedance, a pull-up that decides, a bus with its own timeout). The model is only as true as the data sheet it was filled from, and **the real answer is found by killing the controller on the bench and watching the wheels**, which no header can do. It does not choose a brake, a contactor or a watchdog chip, and does not replace one. The voltage of 12 V is an example. And it covers the stop of the machine, not the energy stored in it: a robot arm with a payload held by its motors falls when they let go, and the answer there is a different brake and a different hazard.

## What Breaks First

- **An output stage left on its default.** It holds. See `E-SAFE-0007`.
- **A zero that is not a stop.** See `E-SAFE-0008`.
- **A brake that needs power to stop.** See `E-SAFE-0009`.

## Ship It

`rc/safety/failsafe.hpp` holds `FailPolicy`, `Channel`, `output_on_failure`, `unsafe_on_failure`, `BrakeDesign` and `brake_engaged`.
