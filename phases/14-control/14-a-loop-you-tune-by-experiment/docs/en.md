# A Loop You Tune by Experiment: Relay Feedback and the Four Numbers That Prove It

> You do not have a model of the motor. You have the motor, a few minutes, and a way to make it tell you what it is.

**Type:** Capstone
**Time:** about 150 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** simulated
**Prerequisites:** 14-04, 14-12

## The Problem

Every lesson in this phase that tuned a loop knew the plant: a mass, a damping, a delay. A new motor on a new joint arrives with none of that, and the choice is to guess gains or to ask the machine.

The classic question to ask is: how hard can you push before it will not stop swinging? Raise the proportional gain until the loop sustains an oscillation, note the gain and the time of a cycle, and everything follows. Doing that by hand means running a loop at the edge of instability on real hardware, which is how motors get hurt.

## The Concept

### A relay does it safely

A **relay** is a controller with two outputs, plus d and minus d. Command plus d while the speed is below the target and minus d while it is above. The speed has to swing about the target, and the push never exceeds d, so nothing runs away.

The swing's size and speed carry the two numbers wanted. If the swing has an amplitude a, the gain at which a proportional loop would just sustain it, the **ultimate gain**, is

```cpp
ku = 4.0 * d / (pi * a);
```

and the time of one cycle is the **ultimate period** tu. See `E-CTRL-0036`.

### The plant, and the check

The test plant is a motor speed loop: the command becomes a speed through three lags in series, 0.5 s of mechanics, 0.2 s of electrics and 50 ms of sensor filter, with a gain of 3. The test works out the true ultimate gain and period from those constants, which a tuner would never see, to check the relay against.

| | relay | worked out from the plant |
|---|---|---|
| ultimate gain | 5.71 | 6.42 |
| ultimate period | 0.544 s | 0.513 s |

Within 11 percent and 6 percent, from one run of a few seconds. The relay answer is an approximation: it treats the square wave of the relay as if it were its fundamental sine, and the other harmonics leave a small error. See `E-CTRL-0036`.

### Not on how hard it pushes

For a plant that is linear the answer should not depend on d, and it does not:

| relay size | amplitude | ultimate gain | period |
|---|---|---|---|
| 0.25 | 0.0558 | 5.71 | 0.544 |
| 0.50 | 0.1116 | 5.71 | 0.544 |
| 1.00 | 0.2231 | 5.71 | 0.544 |
| 2.00 | 0.4463 | 5.71 | 0.544 |
| 4.00 | 0.8925 | 5.71 | 0.544 |

So d is chosen for the machine: small enough that the speed stays in a safe range, large enough that the swing is above the sensor's noise. A real plant is not linear, and a different d may give a different answer there, which is worth knowing.

### The margin

A relay with no margin flips on every noisy reading. A small margin, the **hysteresis**, makes it wait until the error is clear of zero. Too much bends the answer:

| margin | amplitude | ultimate gain | period |
|---|---|---|---|
| 0.01 | 0.2231 | 5.71 | 0.544 |
| 0.05 | 0.2899 | 4.39 | 0.618 |
| 0.10 | 0.3627 | 3.51 | 0.692 |
| 0.20 | 0.4820 | 2.64 | 0.804 |

The gain it reported fell by more than half. Keep the margin to a small fraction of the swing it makes, and above the sensor's noise. See `E-CTRL-0037`.

### From two numbers to three gains

**Ziegler and Nichols** turned thousands of experiments into a rule: Kp is 0.6 Ku, the integral time is Tu / 2 and the derivative time is Tu / 8, with ki = kp / Ti and kd = kp Td. It is aimed at a quarter amplitude decay, which is lively. A gentler set, for some overshoot, is Kp of 0.33 Ku, Ti of Tu / 2 and Td of Tu / 3.

### The four numbers

Whatever rule gave the gains, the test is a step. From Ku 5.71 and Tu 0.544, a step to 1:

| rule set | kp | ki | kd | rise (s) | overshoot | settling (s) | final error |
|---|---|---|---|---|---|---|---|
| Ziegler-Nichols | 3.42 | 12.59 | 0.23 | 0.135 | 0.597 | 1.574 | 0.0000 |
| some overshoot | 1.88 | 6.92 | 0.34 | 0.226 | 0.376 | 1.943 | 0.0000 |

The classic rule is faster and overshoots by nearly 60 percent. The gentler one is slower and overshoots less, and neither leaves any error. The phase promised a loop tuned with measured numbers rather than a feeling. These are the numbers: rise time, overshoot, settling time and final error. See `E-CTRL-0038`.

## Build It

`relay_test`, `ziegler_nichols` and `some_overshoot` in `exercise/solution.hpp`. The plant, `SpeedDrive`, is given.

```
rcpp verify 14-14
```

The simulation is deterministic, so the figures above are the same on every run.

## Use It

`rc/control/autotune.hpp` joins `rc::control`.

```cpp
const RelayResult found = relay_test(1.0, 0.01);
if (found.valid) {
  const PidGains gains = some_overshoot(found.ultimate_gain, found.period);
  // hand them to a Pid, then run a step and look at the four numbers before trusting them
}
```

**What this does not do.** The simulated plant is used directly in `relay_test`, so on a real machine the loop that drives the relay is yours to write, with the actuator limits of lesson 14-02 around it. The relay finds one operating point; a plant whose behaviour changes with the pose
needs a table of gains (lesson 14-13). It does not know the loop's sample period, which has to be far shorter than Tu (lesson 14-12), and a noisy sensor needs the margin set above its noise and a filtered derivative (lesson 14-11). The rules suit plants that lag and
do not integrate; for a plant that does, measure first and expect more overshoot.

## What Breaks First

- **Taking the relay's numbers as exact.** They are an approximation. See `E-CTRL-0036`.
- **A margin too wide.** The ultimate gain reads low. See `E-CTRL-0037`.
- **Gains used without a step.** The rule sets differ by 0.2 in overshoot. See `E-CTRL-0038`.

## Ship It

`rc/control/autotune.hpp` holds `RelayResult`, `relay_test`, `PidGains`, `ziegler_nichols`, `some_overshoot` and the given `SpeedDrive`.

This ends phase 14: a loop you can tune, with four measured numbers rather than a feeling.
