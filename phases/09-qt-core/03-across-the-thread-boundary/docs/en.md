# Across the Thread Boundary: What a Queued Connection Actually Does

> Lesson 10-03 ended with a warning it could not act on: a widget may only be
> touched from the thread that owns it, and the crossing is what signals and
> slots are for. This is the crossing.

**Type:** Build
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 09-02, 07-01
**Needs Qt:** yes

## The Problem

A sensor produces readings on its own thread, because it has to: it is reading a
port, or waiting on a device, and doing that on the thread drawing the interface
would freeze the interface.

The readings have to reach a chart. And the chart is a `QWidget`, which **may
only be touched from the thread that owns it**.

Writing to it from the sensor thread is undefined behaviour. It also works,
almost always, for a long time, which is why it reaches production. The
symptoms when it stops working are a repaint at the wrong moment, an occasional
crash inside Qt with a stack nobody wrote, and a container that is sometimes
empty when it plainly should not be.

Lesson 07-01 met this problem without Qt and answered it with a mutex; 07-02
answered it with a lock free queue. Qt has an answer built into the mechanism
you already use for everything else.

## The Concept

### The connection decides where the slot runs

```cpp
QObject::connect(sensor, &Sensor::reading, log, &Log::take, Qt::AutoConnection);
```

`AutoConnection` is the default, and it decides **at emit time**:

| | what happens |
|---|---|
| sender and receiver on the same thread | a direct function call, no copy, done before `emit` returns |
| on different threads | the argument is copied and posted to the receiver's event loop |

In the second case the slot runs **on the thread that owns the receiver**. That
is the whole mechanism. The sensor never touches the log, and the log's members
are only ever touched from the log's own thread.

Measured: a reading produced on a worker thread is consumed on the thread that
owns the log, five hundred times out of five hundred.

`Qt::DirectConnection` across threads is the bug this lesson is about. It runs
the slot on the emitting thread, immediately, so the receiver's members are
touched from the wrong place and Qt does not intervene.

### Affinity belongs to the object

```cpp
sensor.moveToThread(&worker);
```

That is the only line about threads in the whole arrangement. Neither object has
to be told about the other's thread, and the connection does not change.

Which is what makes the pattern compose: two sensors on two threads reach one
log without any of the three knowing about the others.

### Nothing arrives while the receiver's loop is not running

A queued delivery is an event posted to the receiver's event loop. If that loop
is not running, the readings are **neither lost nor received**. They are
waiting.

This lesson has a test that watches exactly that: the worker finishes producing,
the log has zero, and then the loop runs and the log has twenty.

It is the shape of a great many bugs that present as a dead sensor, and knowing
it turns "the data never arrives" into "what is my main thread doing".

### The registration ritual, and what it hides

Every Qt 5 tutorial says to call `qRegisterMetaType` for anything sent across a
queued connection, because the argument has to be copied and the meta object
system has to know the type.

On Qt 6 that is mostly obsolete, and measuring it is worth the minute:

```text
a plain struct, no Q_DECLARE_METATYPE, no qRegisterMetaType
  -> 500 of 500 delivered
```

A connection made with the function pointer syntax registers the type itself.

**The obsolete advice hides the real hazard**, which is what the failure looks
like when it does happen. Some types still fail, typically ones Qt records under
a name it cannot resolve, such as a typedef:

```text
a signal carrying Qt::HANDLE
  -> 0 of 100 delivered

QObject::connect: Cannot queue arguments of type 'Qt::HANDLE'
```

`connect` returned true. The signal was emitted a hundred times. Every delivery
was discarded, and the only evidence is one line on the console that nothing in
the program can see.

So the durable rule is not "always register". It is:

**A connection that was made is not a connection that delivers.** Assert that
something arrived, once, in a test, for every signal that crosses a thread.

## Build It

Implement `wire` and `Log::take` in `exercise/solution.hpp`.

```
rcpp verify 09-03
```

The suite runs a real worker thread and a real event loop. Every wait is
bounded, so a mistake is a failing test rather than a suite that hangs.

## Use It

This is the shape for every sensor in the rest of the curriculum: the thing that
talks to hardware lives on its own thread and emits, and the thing that draws
receives. The `Link` from phase 08 goes on the worker; the `LivePlot` from 10-03
receives.

It is worth knowing what it costs. A queued delivery allocates and copies, and
at a few hundred a second that is nothing. At a hundred thousand a second it is
not, and that is where the lock free queue from lesson 07-02 belongs, with a
timer on the receiving side draining it. Both mechanisms are in this curriculum
because both are the right answer somewhere.

## What Breaks First

- **A direct connection across a thread boundary.** The slot runs on the wrong
  thread and it usually works. See `E-QT-0011`.
- **A queued argument dropped in silence.** See `E-QT-0010`.
- **A shared value written from two threads.** The problem underneath all of
  this. See `E-THREAD-0001`.

## Ship It

`Sensor` and `Log` join `rc::qt` as the worker pattern the rest of the
curriculum reads sensors through, and phase 09 now covers the three things Qt's
object model is for: what a `QObject` is, what `moc` writes for it, and what a
connection does when the two ends are not on the same thread.
