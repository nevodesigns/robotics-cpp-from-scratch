# The Station That Closed Badly: Shutting Down in the Reverse of Starting, Past a Step That Fails, Once, and the Deletes That Never Ran

> Close things in the reverse of the order they were started, each one even if another failed, and only once.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 12-04, 09-04, 07-07

## The Problem

A station starts the settings, the log, the link, the heartbeat monitor and a plan, in that order, and each depends on what came before it. When it ends it closes them in some order, and the wrong order closes a thing that something still running is using: the last message goes to a log that has been closed, the link is closed under a thread that is reading it.

## The Concept

### Reverse order

Steps added as settings, log, link and monitor were closed as **monitor, link, log, settings**: each closed before the thing it used. What ran was what the report said ran. Each step is added when the thing it closes is started, so a thing that failed to start was never added, and nothing is closed that was never opened: with two steps added, two were closed. See `E-QT-0120`.

### A step that fails

Five steps, with a step that threw `socket busy`, one that returned false, one that threw an `int`, and one that stopped the robot, which came first in the reverse order: all **5** ran, the last included, and **3** were in the report of failures, each by name and, for a throw, with the reason. The step after a failing one may be the one that stops the robot. See `E-QT-0121`.

### Once

Three requests to quit: the step ran **1** time, and the reports had **1**, **0** and **0** steps. A program is asked to quit more than once (a signal, a window close and an exit of the loop), and a second close of a closed link is an error of its own. A sequence with nothing added has nothing to do and is done.

### deleteLater

An object told `deleteLater()`, and then `QCoreApplication::processEvents()`: it was **still alive**. A deferred delete is delivered by an event loop, and not by a call from outside one. After `drain_deferred_deletes()`, which sends the deferred deletes, it was **gone** and the count of live objects was 0. An object that waits for a loop that will not run again is an object whose destructor never runs, with whatever it was holding. See `E-QT-0122`.

## Build It

`ShutdownSequence::run` and `drain_deferred_deletes` in `exercise/solution.hpp`. `add` is supplied.

```
rcpp verify 12-11
```

## Use It

`rc/qt/shutdown.hpp` joins its module.

```cpp
rc::qt::ShutdownSequence shutdown;
settings_ok = open_settings();   shutdown.add("settings", [&] { return settings.save(); });
log_ok = open_log();             shutdown.add("log", [&] { return log.close(); });
link.start();                    shutdown.add("link", [&] { link.stop(); return true; });
robot.arm();                     shutdown.add("robot stop", [&] { return robot.stop(); });
QObject::connect(&app, &QCoreApplication::aboutToQuit, [&] { shutdown.run(); rc::qt::drain_deferred_deletes(); });
```

**What this does not do.** It closes in order and does not know why an order is right: the order is the order of the steps being added, so a step added in the wrong place is closed in the wrong place. It does not wait for a step that does not return, and a step that hangs hangs the shutdown, so a step that waits should carry its own time limit (lesson 12-01 has one). It is for a program that is ending, and a part that is closed and restarted while the program runs wants its own start and stop. It cannot stop a robot that has stopped listening: a step of the station is a request, and the robot has to stop on its own when the station goes quiet.

## What Breaks First

- **Closing in the order that started them.** See `E-QT-0120`.
- **One step that fails and the rest never run, or a second quit.** See `E-QT-0121`.
- **A delete that waits for a loop that does not come.** See `E-QT-0122`.

## Ship It

`rc/qt/shutdown.hpp` holds `ShutdownReport`, `ShutdownSequence` and `drain_deferred_deletes`.

The capstone of this phase ends with one.
