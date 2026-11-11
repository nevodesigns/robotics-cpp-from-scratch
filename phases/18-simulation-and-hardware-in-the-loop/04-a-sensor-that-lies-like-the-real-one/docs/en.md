# A Sensor That Lies Like the Real One: Bias, Noise, Steps, Delay and Dropouts, in the Order They Happen

> A perfect simulated sensor is the most dangerous thing in a test suite. It is the one fake that is always wrong.

**Type:** Build
**Time:** about 120 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 18-02, 15-08, 15-07, 03-02

## The Problem

The plant of lesson 18-02 reports its position exactly, to every digit, at the instant it is asked. No sensor does that. A real one is offset a little, jitters, can only report in steps, can only report within a range, tells you what happened some milliseconds ago, and sometimes does not tell you at all. Phase 15 measured each of these on its own. A controller tested against a perfect simulated sensor has never met any of them.

The fix is to put them in the simulation, one option at a time, from a seed, so that a test can say "this sensor is 60 milliseconds late" and mean it, and so that the same sensor is the same every run.

## The Concept

### What can be wrong

```cpp
struct SensorOptions {
  double bias, noise;          // added to every reading, and the standard deviation of the noise
  double low, high;            // the range
  double step;                 // the resolution
  int delay;                   // samples old
  double dropout;              // the chance a reading is lost
  std::uint32_t seed;
};
```

All zero, with the range open, is a perfect sensor: it read the truth, exactly, for truths of 0, 1.5, -3.25 and a million. A bias of 0.3 on a truth of 2.0 read **2.3000 every time**, with no spread. Noise of 0.05 read a mean of 1.9997 and a spread of **0.0502**: the standard deviation asked for.

### The order matters

A real sensor does these things in a sequence, and a model that does them in another gives different readings:

1. the truth, plus the **bias**, plus the **noise**;
2. held to the **range**;
3. rounded to the **resolution**;
4. made **old** by the delay;
5. and last, the whole reading may be **lost**.

Noise before rounding means the noise decides which step it lands on, and the rounding is the last thing that happens to the number. Rounding before noise would give readings that are not on the grid at all. A reading held to a range of -5 to 5 read 5.0 for a truth of 100 and -5.0 for -100, and 1.25 for a truth of 1.25 that was inside it. A resolution of 0.1 gave readings that were all multiples of it, and an rms error of **0.02885**, against 0.1 / sqrt(12) = **0.02887**: the quantization
error that phase 15 predicted. See `E-SIM-0010`.

### Old readings

A delay of 3 samples, read on a ramp of 0, 1, 2, ...:

```
read 0 0 0 0 1 2 3 4
```

After the history has built up, each reading is the truth three samples ago. Until then there is nothing older, and the sensor gives its first reading, which is what a real sensor does when it has been switched on and the first value is the only one there is. See `E-SIM-0012`.

### Lost readings

A dropout chance of 0.1 lost 2052 of 20000 readings, 0.103. A lost reading is **marked**, `valid` is false, and it carries the last good value and not a zero: a zero is a reading of zero, and a control loop that took it would conclude that the robot had jumped to the origin. The model counts the losses, 2052 of them, and the count agreed with the marks. See `E-SIM-0011`.

### Repeatable

Every reading draws its random numbers whether or not the options use them, so that two sensors with the same seed and the same noise give the same readings, and **switching on a delay does not change the noise**: the delayed sensor in the test read, from the third sample on, exactly what the plain one had read two samples before. Another seed gave a different sensor. Without that, adding a delay to a test would also change its noise and the test would break for a reason that has nothing to do with delay. See `E-SIM-0010`.

### What a perfect sensor hid

The same proportional controller, asked for position 1, on the motor of lesson 18-02:

| sensor | overshoot | swing in the last half second |
|---|---|---|
| perfect | 0.1945 | 0.0000 |
| 60 ms old | 0.6543 | 0.0325 |
| 60 ms old, noisy, coarse | 0.6574 | 0.0305 |

The delay made the overshoot **three times as large**, and the loop is still swinging two and a half seconds after it started. The controller is the same and the plant is the same, and the only change is a sensor that is 12 samples behind. A test run on the first row would have passed and the controller would have rung on the real thing. The noise and the coarse steps added little here, since the delay is what mattered: which is a finding about this controller, and the way to
find out which effects matter is to switch them on one at a time. See `E-SIM-0012`.

## Build It

`read` in `exercise/solution.hpp`. The options, the reading type and the random number generator are given.

```
rcpp verify 18-04
```

The noise comes from a generator seeded in the options, so the figures above are the same on every run.

## Use It

`rc/sim/sensor_model.hpp` joins `rc::sim`.

```cpp
rc::sim::SensorOptions o;
o.delay = 12;  o.noise = 0.005;  o.step = 0.01;
rc::sim::SensorModel encoder(o);
const rc::sim::Reading r = encoder.read(plant.position());
if (r.valid) use(r.value);
```

**What this does not do.** The noise is Gaussian and white, and a real sensor's is often neither: it drifts with temperature and has spikes (lesson 15-07). The bias is constant, where a real one wanders. The delay is a whole number of samples, and a real one is a fraction and varies (lesson 15-03). Dropouts here are independent, and real ones come in runs. The numbers in the options are the user's to find: measuring them on the real sensor is the same job as lesson 18-03 did for the motor.

## What Breaks First

- **A perfect simulated sensor.** A controller that rings on the real one. See `E-SIM-0012`.
- **Effects applied in the wrong order.** Readings that no real sensor would give. See `E-SIM-0010`.
- **A lost reading read as zero.** A robot that jumps to the origin for a cycle. See `E-SIM-0011`.

## Ship It

`rc/sim/sensor_model.hpp` holds `SensorOptions`, `Reading` and `SensorModel`.

A simulation can now give a controller a sensor with the faults of the real one, one switch at a time.
