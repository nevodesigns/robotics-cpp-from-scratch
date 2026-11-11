# The Silence That Was a Failure: A Link That Is Up and Says Nothing, and a Watchdog That Notices

> A link that is up is not a link that is alive. Ask how long ago the robot last said anything, on a clock that goes forward, and say it once when the answer changes.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 12-01, 09-11, 07-14

## The Problem

Lesson 12-01 reconnects when a link closes. A robot that hangs does not close its link. The cable behind a switch comes out, the power goes, a process stops, and the socket at the station stays open for hours with nothing arriving. A station that waits for a close waits for ever, and the person at the screen watches a number that has not changed.

## The Concept

### Connected is not alive

A server that accepted a connection and said nothing, for **300** ms: the socket was **connected** and **0** bytes had arrived. Nothing in the socket says that anything is wrong. The only evidence of life is that the other end said something recently. See `E-QT-0099`.

### A watchdog

`Watchdog` is given the time and reads no clock, so a test can give it any times at all. Started at 1000 ms with a late limit of 100 and a lost limit of 300:

| time | health |
|---|---|
| 1000 to 1099 | alive |
| 1100 (exactly the limit) | **late** |
| 1299 | late |
| 1300 | **lost** |
| 900000 | lost |

A sign of life at 1400 made it alive again, late at 1500 (an age of exactly 100). A link that never said anything starts with the grace of its limits and does not begin lost. With the limits equal, or backwards, there is no late state: alive, then lost.

### A clock that goes back

The last sign at 10000 and a time of 9000, as when a wall clock is put back a second: the age is **0**, and the link is alive; an age that is negative, compared with a limit, is a link that is never lost and was never late. An hour later it is lost, with an age of **3600000**, and an age of four trillion milliseconds does not overflow. The monitor uses `QElapsedTimer`, which is monotonic. See `E-QT-0100`.

### Once for each change

`HeartbeatMonitor` looks every 10 ms. A link that said nothing, with limits of 60 and 150: late after **60** ms and lost after **150** ms, never before, and **2** signals however many times it looked at a link that stayed lost. A `beat()` made it alive again at once, without waiting for the next look. A robot that sent every 25 ms for 350 ms caused **no** changes, and when it stopped the changes were late then lost, in order. See `E-QT-0101`.

## Build It

`Watchdog::age` and `health`, and `HeartbeatMonitor::beat` and `look`, in `exercise/solution.hpp`.

```
rcpp verify 12-02
```

## Use It

`rc/qt/heartbeat.hpp` joins its module.

```cpp
rc::qt::HeartbeatMonitor robot(200, 1000, 50);       // late at 200 ms, lost at one second
QObject::connect(&robot, &rc::qt::HeartbeatMonitor::health_changed, this, [this](int h) { show_robot_health(h); });
robot.start();
// whenever a message arrives from the robot:  robot.beat();
```

**What this does not do.** It says that the robot has gone quiet, and not why: a robot that is busy and one that has died look the same until a command is sent. The limits are the choice of whoever uses it, and they have to be longer than the longest gap that a healthy robot leaves, which a measurement of the real link (lesson 07-16 is how) gives and a guess does not. It judges by one stream of messages, and a robot that sends sensor data but whose controller has hung is alive to it. And it is on the station, so a station that hangs cannot say that it has: a robot that stops moving when it stops hearing is the other half of this.

## What Breaks First

- **A link that is open and silent.** See `E-QT-0099`.
- **A clock that goes back.** See `E-QT-0100`.
- **A change reported at every look.** See `E-QT-0101`.

## Ship It

`rc/qt/heartbeat.hpp` holds `Health`, `Watchdog` and `HeartbeatMonitor`.

The capstone of this phase shows the state of the robot on a screen, and this is where the state comes from.
