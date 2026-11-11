# The Sensor That Never Touched the Screen: A Thread of Its Own, One Way Across and a Stop That Joins

> The sensor runs on a thread of its own, and the only thing it ever calls is the one function that is safe
> from any thread. The interface hears of it on its own thread, and closing the program joins the sensor first.

**Type:** Build
**Time:** about 180 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none
**Prerequisites:** 09-13, 09-11, 09-03, 07-16

## The Problem

Phase 09 has taught the pieces: a value can cross a thread boundary (09-03), a fast sensor needs a latest value that counts what it replaced (09-13), a timer is a grid and not a promise (09-11), and a thread is a promise to join (07-07). This wires them into the rig the phase promised: a sensor on a thread of its own that reaches the interface without ever touching it.

## The Concept

### One way across

The producer thread is a loop: make a reading, call `publish`, sleep for the period. It owns no interface object and calls nothing of one. A 1 kHz sensor for 100 ms produced on **another thread**, and the update was shown on **the interface's thread**. Readings produced equalled readings published, and updates plus skipped equalled both. See `E-QT-0051`.

### A stop that joins

`stop()` asks with `requestInterruption()` and waits with `wait()`. The destructor calls it. A bridge stopped after 50 ms produced the same count fifty milliseconds later. **20** bridges destroyed while running shut down cleanly, and one never started could be stopped twice. See `E-QT-0052`.

### A rate and a prompt stop

A 100 Hz sensor produced no more than the run's length over 10 ms plus 2 readings, and stop was answered in well under 500 ms, since the loop looks at the request every pass. See `E-QT-0053`.

## Build It

`start`, `stop` and the producer loop in `exercise/solution.hpp`. The bridge uses `LatestValue` from `rc/qt/latest_value.hpp` (lesson 09-13).

```
rcpp verify 09-14
```

## Use It

`rc/qt/sensor_bridge.hpp` joins `rc::qt`.

```cpp
rc::qt::SensorBridge bridge(200);                   // 200 Hz
connect(&bridge, &rc::qt::SensorBridge::shown, this, [&](double v, long long skipped) { show(v); });
bridge.start();
// ... and on exit, which the destructor does anyway:
bridge.stop();
```

**What this does not do.** It produces a counter and not a sensor: a real driver replaces `produce_one`. The loop sleeps for a whole period after each reading, so the rate is a ceiling and not a grid, and a rate that must hold is lesson 07-08's pacer or lesson 09-11's ticker. It carries one `double`. And it tells the interface about a reading at the speed of the interface's loop: a blocked loop shows the newest reading late, and says how many it replaced.

## What Breaks First

- **A thread that calls the interface.** See `E-QT-0051`.
- **A thread that outlives its owner.** See `E-QT-0052`.
- **A loop that cannot be stopped.** See `E-QT-0053`.

## Ship It

`rc/qt/sensor_bridge.hpp` holds `SensorBridge`.

This is what phase 09 was for. Phase 10 puts widgets on the other side of it.
