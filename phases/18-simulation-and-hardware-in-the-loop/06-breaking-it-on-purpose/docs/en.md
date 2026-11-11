# Breaking It on Purpose: Injected Sensor Faults and the Monitor That Has to Notice Each

> A safety monitor that has never seen a fault is a guess about what faults look like.

**Type:** Build
**Time:** about 130 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 18-04, 18-05, 17-08

## The Problem

Lesson 18-04 gave the simulated sensor its everyday faults: noise, delay, a coarse step. Those are how a sensor is when it is working. A robot also has to survive a sensor that is **broken**: a cable that has come loose and left the last reading frozen, an encoder that slipped on its shaft, a sensor whose zero has crept. The code that decides what to do about it is the most important code in the robot and the code least often run, since the real fault happens once a year and not
in front of anyone.

A simulator is the only place to run it every day. The faults are switched on at a chosen time, by a line of a scenario (lesson 18-05), and a monitor either notices or does not. This lesson builds four faults, two monitors, and measures how long each takes to notice each fault.

## The Concept

### Four ways to break

```cpp
enum class FaultKind { none, stuck, offset, drift, spike };
```

Each starts at a given time and the readings before it are untouched:

- **stuck** freezes at the last good reading. A sensor reading 2.0 and then asked for 3.0 and 9.0 after the fault gave **2.0, 2.0, 2.0**.
- **offset** adds a fixed amount: 2.0 became 2.5 for an offset of 0.5.
- **drift** adds an amount that grows: 0.1 a second, so 2.0 read 2.2 two seconds in and 3.0 after ten.
- **spike** adds a large amount to one reading in 25: four spikes in a hundred readings, and the rest right.

See `E-SIM-0016`.

### A residual

A monitor needs something to compare the sensor with. Here it is a **shadow** of the motor, a second copy of the plant model driven by the same commands. The difference between the sensor and the shadow is the **residual**, near zero when all is well and not when the sensor lies. The `ResidualMonitor` raises an alarm when the residual has been beyond a `threshold` for `persist` readings in a row, so that one reading over the line is not an alarm.
A run of three beyond 0.1 was, two was not, a reading back inside started the count again, and the alarm dropped the moment a reading came back. See `E-SIM-0017`.

### How long each takes to notice

A position loop with noise of 0.002, a residual threshold of 0.02 for 3 readings, a fault at 2.0 s and a second target at 5.0 s:

| fault | residual alarm | after the fault | stuck monitor |
|---|---|---|---|
| none | none | | never |
| offset of 0.3 | yes | 0.02 s | never |
| drift of 0.05 a second | yes | 0.46 s | never |
| spike of 0.4, one in 25 | **never** | | never |
| stuck | yes | **2.85 s** | at the move |

The healthy run raised no alarm. The offset was caught at once. The drift took 0.46 s, about the threshold over the rate, 0.02 / 0.05 = 0.4, plus the three readings. The spike was **never** caught: a spike is one reading, a run of three never forms, and the monitor was built to ignore a single reading. Whether that is right depends on whether a lone spike is harmful, and a different monitor handles those (lesson 15-07's Hampel filter).
See `E-SIM-0018`.

### The one that hides

A stuck sensor froze at about the target, so the loop commanded almost nothing, the plant barely moved, and the residual stayed small. The residual monitor caught it 2.85 s late, and only because the robot's own slow creep opened a gap. The **stuck monitor** has a different idea: a reading that has not changed for 25 readings *while the robot is expected to be moving* is stuck. It did not fire while the robot was at rest, since an unchanging reading is right then, and it fired at the second target, when the robot was asked to move.
An unchanging reading with no motion expected gave no alarm in 20; with motion expected it fired at the 5th. Each monitor sees what the other cannot. See `E-SIM-0018`.

### Where to set the line

The threshold is a trade, and the simulation can say what it costs. 200 healthy runs of 10 s with noise 0.002, the number of runs with a false alarm:

| threshold | persist 1 | persist 3 |
|---|---|---|
| 0.004 | 200 | 16 |
| 0.006 | 178 | 0 |
| 0.010 | 0 | 0 |
| 0.020 | 0 | 0 |

Set at twice the noise, every healthy run raised an alarm. Asking for three in a row took the 16 down to none by 0.006. And a drift of 0.05 a second from 2.0 s, the alarm by threshold:

| threshold | alarm after the fault |
|---|---|
| 0.02 | 0.46 s |
| 0.05 | 1.06 s |
| 0.10 | 2.05 s |
| 0.20 | 4.04 s |

A threshold ten times higher found the drift **nine times later**, by which time the robot's position had been wrong by 0.2. The line has to be above the noise and as low as the false alarm count allows, and both numbers come from running the simulation. See `E-SIM-0017`.

## Build It

`distort` of `FaultInjector`, `update` of `ResidualMonitor` and `update` of `StuckMonitor` in `exercise/solution.hpp`. The constructors and the fault kinds are given.

```
rcpp verify 18-06
```

The noise comes from generators seeded in the test, so the figures above are the same on every run.

## Use It

`rc/sim/faults.hpp` joins `rc::sim`.

```cpp
rc::sim::FaultInjector fault;
fault.set(rc::sim::FaultKind::drift, 2.0, 0.05);          // from a line of the scenario
const double reading = fault.distort(sensor.read(plant.position()).value, t);
if (monitor.update(reading - shadow.position())) enter_safe_state();
```

**What this does not do.** The shadow is a perfect copy of the plant, which is what makes the residual small, and a real model has an error of its own that eats into the threshold (lesson 18-03 measured it). A push on the plant that the shadow does not know about looks exactly like a fault in the sensor: the monitor cannot tell which of the two lied. It detects and does not decide: what the robot does on an alarm, and how fast, is lesson 17-20's
supervisor and a safety analysis. And four faults are a start, not a list: a real sensor can also fail in ways that depend on temperature or on speed.

## What Breaks First

- **A monitor that has never seen the fault.** One that does not notice it, or notices too late to matter. See `E-SIM-0018`.
- **A threshold set by feel.** False alarms every run, or a drift found at five times the error. See `E-SIM-0017`.
- **A stuck sensor at rest.** A frozen reading that looks exactly right. See `E-SIM-0018`.

## Ship It

`rc/sim/faults.hpp` holds `FaultKind`, `FaultInjector`, `ResidualMonitor` and `StuckMonitor`.

A robot's safety code can now be run every day against the faults it exists for.
