# Whose Clock Is It: Simulated Time, Jumps and the Timer That Fires 200 Times at Once

> A timer on the wall clock is a timer on the real world. A timer on a simulator's clock is a timer on whatever the simulator says time is, and it is allowed to say anything.

**Type:** Build
**Time:** about 110 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 17-08, 03-05

## The Problem

Lesson 3-05 made a clock a thing that could be replaced, so a test could say what time it was. Lessons 17-04 and 17-08 took the time as an argument for the same reason. This lesson is the case where that stops being a convenience and becomes the system.

A simulator runs the robot faster than real time, or slower, or paused while you look at something. It restarts. It jumps ahead past a boring part. Every node in the simulation must follow the **simulated time**, which arrives as messages on a topic, and not the machine's wall clock, or the planner and the simulated world will disagree about how long a second is. ROS 2 calls it `use_sim_time`.

## The Concept

### A clock that does not tick

A **SimClock** has no oscillator. Its time is what the last clock message said:

```cpp
void on_clock(double t);    // the time is now t
```

Everything in a node that cares about time asks this clock. A timer on it behaves in three new ways that a timer on the wall clock never has to.

### Paused and fast

A timer of period 0.5 s fired 20 times over 10 s of clock time, in steps of a tenth. Then the clock stood still: 1000 polls of the same time, and no firing. A timer on the wall clock would have fired 2000 times a second later, and the node would have gone on commanding a robot that the simulator had stopped. See `E-BUS-0037`.

Now run the clock at different speeds, 1000 updates over 10 wall seconds:

| clock speed | clock seconds | firings | per clock second | per wall second |
|---|---|---|---|---|
| 0.5 | 5.0 | 10 | 2.00 | 1.00 |
| 1.0 | 10.0 | 20 | 2.00 | 2.00 |
| 2.0 | 20.0 | 40 | 2.00 | 4.00 |
| 10.0 | 100.0 | 200 | 2.00 | 20.00 |

The timer fires twice per **clock** second at every speed, which is what the controller it drives was tuned for. Per wall second it fires 1, 2, 4 and 20 times. A control loop of 100 Hz running at ten times speed has a period of 1 ms of wall clock, which may be faster than the machine can run it: simulating fast has a limit. See `E-BUS-0037`.

### A jump forward

The simulator skips 100 seconds ahead. A timer of period 0.5 s has 200 periods overdue. A timer that makes up every one it missed fires 200 times at once, 200 control commands in one instant for a time that never happened. A timer allowed to catch up once fires once, and counts the 199 it skipped:

```
a timer allowed to catch up once fired 1 and skipped 199; one that makes up every period fired 200 times at once
```

It then goes on from there: the next firing was at 100.5 s. See `E-BUS-0038`.

### A jump back

The simulation restarts and the clock goes from 100 s back to 0. A timer whose next firing is at 100.5 s would wait for the clock to come round again, and be silent for the whole first 100 seconds of the new run, with a node that looks frozen and a log with nothing wrong in it. The clock counts a jump back and changes its **epoch**, and a timer that sees a new epoch starts again from the new time:

| | fired before restart | in the 100 s after |
|---|---|---|
| timer that looks at the epoch | 200 | 199 |
| timer that keeps its old schedule | 200 | 0 |

The one lost firing is the price of re-arming. Any step back counts, however small: a time of 1.4999 after 1.5 is a jump back, and the same time twice or a small step forward is not. See `E-BUS-0039`.

## Build It

`on_clock` and `poll` in `exercise/solution.hpp`. The accessors are given.

```
rcpp verify 17-13
```

Time is passed in, so the figures above are the same on every run.

## Use It

`rc/bus/sim_time.hpp` joins `rc::bus`.

```cpp
SimClock clock;
SimTimer control(0.01, 1);
// when a clock message arrives: clock.on_clock(t);   in the node's loop:
if (control.poll(clock) > 0) run_controller();
```

**What this does not do.** It does not publish the clock: some node has to be the source, and in ROS 2 it is the simulator. It does not mix wall and simulated time, and a real node on a robot with a simulated part has both. A clock that jumps back also invalidates anything that was stamped in the future and remembered, which the lifespan of lesson 17-08 and the discovery lease of lesson 17-04
both are: they need to be told. And the timers poll, where a real executor sleeps until the next one is due.

## What Breaks First

- **A timer on the wall clock in a simulation.** It keeps firing on a stopped simulator. See `E-BUS-0037`.
- **A timer that makes up a jump.** Two hundred commands in one instant. See `E-BUS-0038`.
- **A timer that ignores a jump back.** Silent for as long as the simulation had run. See `E-BUS-0039`.

## Ship It

`rc/bus/sim_time.hpp` holds `SimClock` and `SimTimer`.

A node can now live in time that somebody else controls.
