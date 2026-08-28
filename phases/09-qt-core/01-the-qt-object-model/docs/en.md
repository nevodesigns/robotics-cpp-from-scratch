# The Qt Object Model: Signals and Slots

> A sensor should be able to announce that something happened without knowing who is listening.

**Type:** Build
**Time:** about 90 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Qt:** 6.2 or newer, the version apt installs on both supported Ubuntu releases
**Hardware:** none
**Prerequisites:** 01-03

## The Problem

Your battery monitor notices the charge has dropped below fifteen percent. Four
things need to know: the screen, so it can show a warning; the logger, so the
incident is recorded; the navigation system, so it can head for the dock; and the
safety layer, so it can refuse new missions.

The obvious approach is for the monitor to call all four. Now the battery
monitor depends on the screen, the logger, the navigator, and the safety layer.
Test it and you have to build all four. Reuse it on a robot with no screen and it
does not compile.

The monitor should announce. Whoever cares should listen. Neither should know
about the other.

## The Concept

Qt's answer is **signals and slots**, and it is the reason Qt is worth learning
even in code that never draws a window.

A **signal** is a function you declare and never write a body for. Qt writes it
for you. Calling it means "this happened":

```cpp
signals:
  void lowBattery(int percent);
```

A **slot**, or in modern Qt any callable at all, is what runs in response.
`QObject::connect` ties them together:

```cpp
connect(&monitor, &BatteryMonitor::lowBattery,
        &screen,  &Screen::showWarning);
```

The monitor knows nothing about the screen. It emits, and everything connected
runs. Nothing connected means nothing happens, which is not an error.

### What Q_OBJECT is doing

For that to work, something has to generate the signal bodies and the machinery
that finds the connections. That something is **moc**, the meta object compiler.
It reads your header, finds `Q_OBJECT`, and writes a C++ file containing the
generated code, which is then compiled alongside yours.

So a Qt class needs three things:

1. It inherits from `QObject`.
2. It has `Q_OBJECT` on the first line of the class body.
3. Its header is known to the build system, so moc sees it.

Miss any of the three and you get a linker error about a vtable or a missing
signal body. That error is so common it has its own atlas entry, and lesson 09-02
is devoted to producing it on purpose and reading it properly.

### Object ownership

A `QObject` can have a parent, passed to its constructor. When the parent is
destroyed it destroys its children. This is Qt's memory management, it predates
smart pointers, and it is why so much Qt code uses raw `new` without leaking.
Parented objects are owned by the tree. Objects on the stack are not, and giving
a stack object a parent is a double delete waiting to happen.

### Hysteresis, which is the actual engineering here

A battery hovering at exactly fifteen percent will cross the threshold up and
down many times a second as the voltage flickers. A monitor that emits on every
crossing produces a stream of warnings and a log nobody can read.

The fix is two thresholds instead of one. Emit the warning when the charge falls
below fifteen. Do not emit again until it has risen back above twenty. The gap
between the two is the hysteresis band, and every real threshold detector on
every real machine has one.

## Build It

`exercise/solution.hpp` gives you a `BatteryMonitor` skeleton. Implement:

- `update(int percent)`, which records the reading and emits `lowBattery(percent)`
  the first time the charge falls to or below the low threshold.
- It must not emit again until the charge has risen to or above the clear
  threshold, at which point it emits `batteryRecovered(percent)` and arms itself
  again.
- `isLow()` reports the current state.

```
rcpp verify 09-01
```

The tests connect lambdas to the signals and count how many times each fires,
which is how you test event driven code without a screen or an event loop.

## Use It

Every Qt program is built from this. Widgets emit `clicked`, timers emit
`timeout`, network sockets emit `readyRead`, and a serial port emits
`readyRead` too, which is how phase 12 talks to hardware without blocking the
interface.

The pattern outside Qt is called publish and subscribe, and you will meet it
again in phase 17 as the core idea of ROS 2 topics. The mechanism differs. The
thinking is identical, and you now have it.

## What Breaks First

- **undefined reference to vtable, or to your signal.** The class is missing
  `Q_OBJECT`, or its header is not part of the target so moc never ran. See
  `E-QT-0001`.
- **connect returns false and nothing happens.** The signal or slot signature
  does not match, or you connected to an object that was already destroyed. See
  `E-QT-0002`.
- **Your monitor never changes state.** You updated a copy rather than the
  object. See `E-CPP-0006`.

## Ship It

`BatteryMonitor` becomes the first component of `rc::qt`, the control station you
assemble across phases 10 to 12. The hysteresis logic inside it moves to
`rc::control`, because it is not a Qt idea at all, it is a control idea that
happens to be announced through a Qt signal.
