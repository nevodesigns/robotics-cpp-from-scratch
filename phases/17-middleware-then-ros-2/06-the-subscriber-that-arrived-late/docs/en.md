# The Subscriber That Arrived Late: Durability, the Last Value and How Old Is Too Old

> A message published once, before anyone was listening, is a message that was never sent. Unless the topic has a memory.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-01, 17-02

## The Problem

Lesson 17-01 left a loose end: a subscriber that arrived after a message missed it. For a stream, that is fine, the next message is a moment away. For some things the next message is never coming. A map is published once, when the mapping node finishes. The robot's description is published once at start up. A goal is set, and then nothing changes for a minute.
A node that starts a second later sees none of it and is lost for ever, or until someone restarts the publisher.

ROS 2 calls the cure **durability**: a topic marked **transient local** remembers its last messages and gives them to anyone who subscribes afterwards. The ordinary topic is **volatile**.

## The Concept

### Remember the last few

A topic is advertised with a durability and a `keep`, how many of the latest messages it holds. A subscriber that arrives is given what is remembered first, oldest first, and then what is published after.

```cpp
bus.advertise("map", Durability::transient_local, 3);
```

Keep 3, five messages published, then a subscriber arrives and a sixth follows: the subscriber heard 3 4 5 6, and 3 were replayed. With a topic that is not remembered, a subscriber that arrives after two messages hears only what comes after it. See `E-BUS-0016`.

### A memory with a bound

The memory is a queue of fixed size: publishing 1000 messages to a topic that keeps 1 leaves 1 remembered. A late subscriber hears the last of them, the 999th, once. A subscriber that was there all along heard all 1000 once each, and the late arrival's replay did not repeat anything to it: the replay goes to the new subscriber only. See `E-BUS-0017`.

### Old news

What is remembered stays remembered. A goal published at 0 s and read by a node starting at 100 s is the goal from a minute and a half ago. The robot may have a different one now, or the mission was cancelled. A subscriber can say how old a replayed message it is willing to take:

| subscriber | at 100 s, remembered from 0 s and 98 s | heard |
|---|---|---|
| any age | both | 2 |
| a limit of 5 s | only the one from 98 s | 1 (1 skipped) |

The skipped ones are counted, so a subscriber can tell that it was given less than was remembered. For a map the age does not matter and for a goal it matters a great deal. See `E-BUS-0018`.

### The three nodes

A map is published at 0 s and three nodes start, at 1, 5 and 9 s:

| topic | nodes that have the map |
|---|---|
| not remembered | 0 of 3 |
| remembered | 3 of 3 |

The order in which a robot's nodes start is not under anyone's control, and a system that depends on it works on the day it was tested. See `E-BUS-0016`.

## Build It

`advertise`, `subscribe` and `publish` in `exercise/solution.hpp`. The counters and `remembered` are given.

```
rcpp verify 17-06
```

Time is passed in, so the figures above are the same on every run.

## Use It

`rc/bus/durable_bus.hpp` joins `rc::bus`.

```cpp
bus.advertise("robot_description", Durability::transient_local, 1);
bus.subscribe("goal", handler, now, 5.0);     // nothing older than five seconds
```

**What this does not do.** The memory is in the bus and it is lost when the bus is: this is durability while a publisher lives, and "persistent" in the DDS sense, surviving a restart, is a different thing. The keep is the same for every subscriber, where the real thing lets each side ask. It remembers the last messages of a topic and not the last per key, so a topic carrying many robots' states has one memory for all of them.
And its replay is immediate and complete: a very large map goes through the handler at once.

## What Breaks First

- **A once-only message on an ordinary topic.** Seen by whoever happened to be listening. See `E-BUS-0016`.
- **A memory with no bound.** It grows as long as the publisher runs. See `E-BUS-0017`.
- **A replay with no age limit.** A goal from yesterday acted on today. See `E-BUS-0018`.

## Ship It

`rc/bus/durable_bus.hpp` holds `Durability`, `Stamped` and `DurableBus`.

A late node now gets what it missed, and can refuse what is too old.
