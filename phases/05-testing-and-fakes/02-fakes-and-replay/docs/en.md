# Fakes and Replay: Testing Code That Talks to Hardware

> The sensor that produced the bug was on a robot, in a warehouse, three weeks ago. You cannot borrow it. You can borrow what it said.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04, Ubuntu 24.04, Windows
**Hardware:** none, which is the entire point
**Prerequisites:** 05-01, 03-03

## The Problem

Everything from here on talks to hardware. A serial port in phase 08, an inertial
sensor and a camera in phase 15, a motor driver in phase 14.

Three things go wrong the moment code touches a device.

**It cannot be tested by anybody without that device.** Half the people reading
this do not have a LiDAR, and the ones who do cannot run the tests on a build
machine.

**It cannot be tested repeatably even by somebody who has one.** A sensor
returns slightly different numbers every time. A test that asserts on real
readings either asserts nothing useful or fails at random.

**The interesting cases refuse to happen on demand.** A test needs the sensor to
drop out, to return a corrupt frame, to answer late, to report a value that
cannot be true. Real hardware does all of those eventually, on its own schedule,
never while you are watching.

The answer to all three is the same, and it is the reason this curriculum can
promise that no lesson is blocked by hardware you do not own.

## The Concept

### Depend on an interface, not on a device

Code that names a concrete device can only ever run with that device. Code that
names an interface can run with anything that satisfies it.

```cpp
class SensorSource {
 public:
  virtual ~SensorSource() = default;
  virtual rc::expected<double, ReadError> read(long at_ms) = 0;
};
```

The real driver implements it by talking to a port. A fake implements it by
answering from a list. The code under test cannot tell, and does not need to.

This is the first genuinely useful thing runtime polymorphism buys you in this
curriculum, and it is worth noticing that the reason is testability rather than
elegance. The virtual destructor is not decoration: deleting a derived object
through a base pointer without one is undefined behaviour, and it is the single
most common mistake with interfaces.

### A fake is not a mock

The words get used interchangeably and the distinction is worth keeping.

A **fake** is a working implementation with a shortcut. A sensor that returns
recorded numbers is a fake: it genuinely answers, it just does not involve
hardware.

A **mock** records how it was called so a test can assert on the interaction:
that `close` was called exactly once, that nothing was read after a failure.

Fakes make tests read like descriptions of behaviour. Mocks make tests read like
descriptions of implementation, which is why a mock heavy test suite breaks every
time the implementation changes even though the behaviour did not. Prefer fakes,
and reach for a mock when the interaction genuinely is the requirement, as it is
for "the port must be closed exactly once".

### Replay is a fake with real data

The best fake is one that answers with numbers a real device actually produced.
Record a session once, keep the trace, and every test afterwards is a
reproduction of a real one.

That is why this repository keeps a `fixtures/` directory, and why rule L014
requires any lesson at hardware tier two or three to ship a fallback. A recorded
trace turns "you need a LiDAR" into "you need a file".

### The failures have to be reachable

A fake that only ever succeeds tests half the code. The interesting half is what
happens when the device does not cooperate, and a fake is the only place those
cases are available on demand.

So a good fake can be told to fail: at a particular reading, with a particular
error, after a delay, or forever from some point on. The exercise builds exactly
that, and the test that matters checks what the code above does when the sensor
stops answering, which on a real robot is the difference between stopping and
driving into a wall.

## Build It

`exercise/solution.hpp` gives you the `SensorSource` interface and the error
type. Implement:

- `ReplaySensor`, which answers from a recorded trace of values, in order, and
  reports `EndOfData` once the trace is exhausted.
- `ReplaySensor::fail_at(index, error)`, making a chosen reading fail instead,
  so a test can reach the case it needs.
- `ReplaySensor::reads()`, how many times it was asked, which is the one piece of
  mock like behaviour worth having here.
- `SensorMonitor`, the code under test, which reads a source and keeps a running
  mean of the good values, counts failures, and goes `stale` after
  `allowed_failures` consecutive failures. A successful read clears the run.

```
rcpp verify 05-02
```

## Use It

Phase 08 gives `SensorSource` a real implementation over a serial port, and the
tests written here keep working unchanged, which is the sign that the seam was
put in the right place.

Larger projects express the same idea with a template parameter rather than a
virtual function, which removes the indirection at the cost of the type being
visible everywhere. Both are the same move: name what you need, not who provides
it.

## What Breaks First

- **A test that passes with the fake and fails on hardware.** The fake is more
  cooperative than the device: it never answers late, never returns a value
  outside its range, never disappears mid read. See `E-TEST-0001`.
- **Deleting through a base pointer with no virtual destructor.** The derived
  part is never destroyed, which leaks quietly. See `E-CPP-0017`.
- **The monitor trusting a source it has not heard from.** Same shape as the
  watchdog: absence of news is not good news. See `E-CTRL-0003`.

## Ship It

`SensorSource` and `ReplaySensor` join `rc::io`, and from here every driver in
this curriculum is written against the interface with a recorded trace beside it.
That is what makes the promise at the top of the README true: no lesson is
blocked by hardware you do not own.
