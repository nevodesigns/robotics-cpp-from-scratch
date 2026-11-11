# A Graph That Looks After Itself: Watching Nodes, Restarting the One That Stalls, and Stopping Safely When That Fails

> Every lesson in this phase made one failure visible. A robot has all of them, one after another, at three in the morning.

**Type:** Capstone
**Time:** about 160 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-19, 17-08, 17-17

## The Problem

A graph of managed nodes can be brought up in order (lesson 17-19). Then it runs, and something in it stops. Not crashes, which is easy to see, but stalls: the camera driver is still up and has stopped delivering, a planner is waiting on a lock, a sensor's cable has worked loose. Everything downstream goes quiet, and whatever commands the arm is now acting on data that is getting older.

This lesson puts the phase together. A graph of four nodes in a chain, a sensor, a filter, a planner and a controller, plus a viewer on the side, all managed nodes (lesson 17-18) on the in-process bus (lessons 17-01 to 17-17), with a **supervisor** that watches their output against a deadline (lesson 17-08), restarts what stalls through its life cycle, and decides when to give up.

## The Concept

### Two layers of safety

There are two defences, and the order they act in matters. The **controller** has a watchdog of its own: if no plan has come for 0.2 s it commands a **stop**, once, whatever anyone else decides. The **supervisor** has a deadline of 0.25 s on the sensor, and on a stall restarts it. The first is fast and local and cannot be argued with, and the second repairs.

A healthy graph for 10 s gave 99 move commands, no stops and **no supervisor events**: the supervisor with nothing wrong has nothing to do. With the sensor stopped for a second it had events at once.

### A stall, detected and repaired

The sensor stops at 5.00 s, active and producing nothing. Its last scan was at 4.91 s.

```
the arm was told to stop at 5.12 s (the controller's watchdog, 0.20 s)
the supervisor saw the sensor stalled at 5.17 s and restarted it at 5.17 s
the arm moved again at 5.21 s; stops commanded: 1; restarts of the sensor: 1
```

The supervisor's detection is a deadline after the last output (0.25 s + 0.01 s of polling), which is not a deadline after the stall began: nobody knows when a silence begins until the next output fails to come. The stop came at 5.12 s, **before** the supervisor acted: the arm was never driven on stale data. Between the stop and the arm moving again there was not a single move command: the controller stops once, and a node that went on publishing "move" from its last plan would have
kept the arm in motion. The restart was a walk through the life cycle: deactivate, cleanup, configure (which reopens the device and clears the fault), activate. See `E-ROS-0010`.

### The cascade

When the sensor stalls, the viewer, which is fed from it through the filter and planner, falls silent too, and by its deadline is just as stalled. A supervisor that restarted everything that missed a deadline would restart the viewer, and the filter and planner too: the victims.

Silence spreads down a graph, so each node is told which watched nodes it listens to. A node whose upstream stalled in the same look is **starved**, and left alone with a fresh deadline: in the stall above the viewer was reported starved 1 time and restarted 0. If the upstream is repaired the starved node resumes by itself and nothing more happens. See `E-ROS-0011`.

### A restart that does not work

The cable is out. The sensor stalls and its device cannot be opened, so each restart fails in configure:

```
5.17 stalled;  5.17 restart_failed;  5.43 stalled;  5.43 restart_failed;  5.69 stalled;  5.69 gave_up
```

A restart that fails is reported as such, and the node is given a fresh deadline, so a restart that did not work is noticed one deadline later: three stalls 0.26 s apart. After the allowed 2 restarts, the third stall is a **give up**, 0.69 s after the stall began. The sensor is required, so the supervisor says the graph cannot be trusted, and the launcher takes it down in reverse: 8 transitions, until the controller and sensor are unconfigured. The arm was told to stop once, early, and nothing moved it after.
The viewer, starved at every one of the three stalls, was never restarted. See `E-ROS-0011`.

### Something that is not worth stopping for

The viewer is optional. When it stalls and cannot be reopened, with one restart allowed, the supervisor tried once, gave up on it once, and the graph carried on:

```
the viewer stalled and could not be reopened: given up on 1 time, graph gave_up() is false, the arm still got 49 move commands in 5 s, 0 stops
```

A give up on an optional node does not make the graph untrustworthy. See `E-ROS-0012`.

### Only the one that stalled

A node is restarted when it stalls, not otherwise. One stall in 6 s gave **1** stalled event, 1 restart of the sensor and 0 of the viewer: a restart gives the node a fresh deadline, so it is not reported again at once, and a node that is never silent is never touched. See `E-ROS-0012`.

## Build It

`tick` and `restart` in `exercise/solution.hpp`. The events, `watch`, `on_output` and the accessors are given. The graph itself (the sensor, filter, planner, controller and viewer, the launcher and the in-process bus) is in the tests and in the library.

```
rcpp verify 17-20
```

The clock is advanced by hand in steps of 10 ms and nothing is random, so the times above are the same on every run.

## Use It

`rc/ros/supervisor.hpp` opens with `rc::ros`.

```cpp
Supervisor supervisor(2);
supervisor.watch("sensor", sensor_life, 0.25, true, {});
supervisor.watch("viewer", viewer_life, 0.25, false, {"sensor"});
// when a node produces output: supervisor.on_output("sensor", now);   every cycle:
for (const SupervisorEvent& e : supervisor.tick(now)) log(e);
if (supervisor.gave_up()) launcher.shut_down();
```

**What this does not do.** It watches output and has no idea why a node stopped: a node stalled by a lock and one stalled by a cable are restarted the same way, which is right for one and wasteful for the other. It restarts at once, where a real supervisor waits longer after each failure so as not to hammer a device. It leaves the upstream lists to the person who wrote the graph, and a graph with a wrong list restarts a victim or leaves a culprit alone. It is in-process and a real graph restarts another
process, which can fail to start. And the safe state here is a stop: for an arm holding a load, or a vehicle on a slope, the right safe state is something else, and that is a safety analysis and not a supervisor (a later phase).

## What Breaks First

- **A stalled node nobody was watching.** Commands on stale data until someone notices. See `E-ROS-0010`.
- **A cascade restarted.** Every node downstream of one fault restarted, and the fault still there. See `E-ROS-0011`.
- **A restart that is retried for ever.** A device hammered, and a robot that never says it is broken. See `E-ROS-0011`.
- **A convenience node that stops the robot.** See `E-ROS-0012`.

## Ship It

`rc/ros/supervisor.hpp` holds `SupervisorEvent` and `Supervisor`.

This ends phase 17: a graph of nodes brought up in order, running, losing one and degrading safely, on a bus written by hand, with the seam where ROS 2 plugs in. The ROS 2 adapter itself is not built or tested here, since this repository's continuous integration has no ROS 2 lane.
