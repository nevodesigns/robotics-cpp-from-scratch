# A Station You Can Ship: A Link, a Watchdog, an Enable Switch, Commands That Are Retried and a Shutdown That Is in Order

> Each part of a station is safe by itself. The station is where they meet: it must be safe when the robot is quiet, when an answer is lost, and when it is closed.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 12-11, 12-10, 12-09, 12-08, 12-05, 12-03, 12-02, 12-01

## The Problem

Phase 12 made one part of a station safe in each lesson: a link that comes back, a watchdog that notices silence, commands that are retried and applied once, an enable key and an interlock, a shutdown in order, a settings file, a log, a command line, a deployment check and an account of the licence. A station is not safe because its parts are. It is safe if they are connected correctly, and each connection is a place where one can be missed: a watchdog whose answer goes nowhere, a retry that sends the wrong speed, a close that leaves a timer running.

## The Concept

### The station

`Station` owns a `LinkClient` (12-01), a `HeartbeatMonitor` (12-02), an `Interlock` (12-08), a `CommandSender` (12-03) and a `ShutdownSequence` (12-11). The robot of this protocol sends `HB` now and then and `ACK <number>` for each command; the station sends `CMD <number> <speed>`. The tests run it against a robot on this machine that does exactly that, can fall silent, and can lose the first answer to each command.

### Driving

The station connected, heard the robot, and its health was alive. With the key up, a request of 1 m/s sent **0**: a zero is a command to stop, and it has to arrive. With the key held, a request of 1 sent **1** and a request of 9 sent **1.5**, the limit; the robot received **3** commands, the last at 1.5, and every one was answered.

### When the robot goes quiet

The robot stopped sending heartbeats: the station said **lost** and the speed was **0**. The robot came back, with the key still held: health was alive, and the speed was still **0**. After the key was let go and pressed, it was **1**. The watchdog does not only report: its answer is connected to the interlock, and the interlock does not start by itself. See `E-QT-0123`.

### When an answer is lost

The robot lost the first answer to a command. The station sent it again, so the robot received **2** copies, and **applied** it **once**; the station had **0** waiting and **0** failed. The retry sends the speed the command had, kept with its number, and not the speed of whatever is asked now. See `E-QT-0124`.

### Closing, and shipping

`stop()` closed `retries`, `monitor` and `link`, the reverse of the order they were started, and was ok; a second `stop()` did nothing. A command asked for after it reached the robot **0** times. And the station is shippable in the two senses of the last lessons: the modules it links, Core and Network, linked dynamically with the licence text and an offer of source, had **0** findings, and a folder that held the station and its libraries passed the deployment check, which reported exactly the missing library when one was removed. A station that has not been started allows no speed. See `E-QT-0125`.

## Build It

`Station::drive`, `on_line`, `on_health` and `retry` in `exercise/solution.hpp`. `start`, `stop` and the rest are supplied.

```
rcpp verify 12-12
```

## Use It

`rc/qt/station.hpp` joins its module.

```cpp
QCoreApplication app(argc, argv);
rc::qt::Station station(QHostAddress("192.168.1.20"), 5000, 1.5);
station.start();
QObject::connect(&app, &QCoreApplication::aboutToQuit, [&] { station.stop(); });
// on the enable key:  station.set_enable(key.held());     to drive:  station.drive(requested_mps);
```

**What this does not do.** It is the wiring of the parts and not all of a station: it has no display, no plan, and it reads no settings or arguments, which the lessons of this phase made and which a real station adds, in the same way, at its edges. Its protocol has three kinds of line and no checksum, so a corrupted line is a line it does not understand and ignores. It does not make a robot safe: the robot has to stop on its own when it stops hearing the station, which is its half of the heartbeat. It has been tested against a robot that was made for the test. And passing these tests is not the same as being safe: a safety case is a different document, and lesson 19 is where it begins.

## What Breaks First

- **A watchdog whose answer goes nowhere.** See `E-QT-0123`.
- **A retry that sends the wrong thing.** See `E-QT-0124`.
- **A station that is not closed, or not shippable.** See `E-QT-0125`.

## Ship It

`rc/qt/station.hpp` holds `Station`.

This is what phase 12 was for: a control station that can be shipped. Phase 13 is where the robot it drives gets an arm.
