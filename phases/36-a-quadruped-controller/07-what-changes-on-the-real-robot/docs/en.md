# What Changes on the Real Robot: Delay, a Slow Actuator and Friction in the Loop, the Delay a Joint Can Stand, and Gains Chosen Against a Spread of Robots

> A controller that works in the simulator has shown only that it works in the simulator; the gap has numbers, and a margin against them can be worked out.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 36-06, 29-07, 18-08, 29-03

## The Problem

Every lesson of phases 34 to 36 was **tested in a simulator**, and the simulator is **kinder than the robot**. The gap is not a mystery: it has a short list of causes (a **delay** between the command and the motor, an actuator that **follows the command slowly**, **friction** and **backlash** in the gears, **noise** in every sensor, **masses and inertias** that are not what the model says, and **contacts** that are not what the model says), and for each there is a **number** that can be **measured** (18-08 compared a simulator with a bench) and a **margin** that can be **worked out**. This lesson does it for one joint, the smallest case that shows the effect.

## The Concept

### A joint with the real robot's differences added

A joint with an **inertia** (**0.05 kg m2**), a **PD law**, a **torque limit** and **friction**, simulated with two **additions**: a **delay** (the command worked out now reaches the motor some milliseconds later, from the bus and the servo's own loop, 29-07) and a **lag** (the torque the motor gives **follows** the command with a time constant). A step from rest to an angle: does it **settle** (stay within 2 percent for the last second), how far does it **overshoot**, when does it settle? With gains of **10 and 1.5** it settled in **0.53 s** with **0.0 percent** of overshoot in the simulator. See `E-LEG-0069`.

### The stiff controller rings

With **stiffer** gains (**40 and 3.0**) the simulator showed a **quicker** joint (**0.24 s**, **0.2 percent** of overshoot). Add **20 ms** of delay and a **10 ms** lag and the same gains **overshot 23.5 percent and did not settle at all**: the delay adds **phase lag** at the frequency where the loop crosses, and **a quick loop has less margin**. **The controller that looked best in the simulator is the one that failed on the robot.** See `E-LEG-0070`.

### How much delay it can stand

By **halving an interval**: the largest delay for which the step still settled was **19 ms** for the stiff gains and **43 ms** for the gentler ones (**10 and 1.5**): **more than twice the margin**, for a joint that is **slower** by a modest factor. **A margin in delay is what the gains buy or spend.** It is a **number to know for every loop**, and to **compare with the delay measured on the real bus** (29-07). See `E-LEG-0071`.

### Against a spread of robots

No two robots are the same. **100 joints** were drawn at random with the **inertia within 30 percent** of the nominal, a **delay from 0 to 30 ms** and a **lag from 0 to 15 ms**: the stiff gains **settled on 46 percent** and the gentle gains on **97 percent**. This is **randomisation**: **design against the spread**, not against the one robot of the simulator. The same method (**draw many, count the successes**) is how the gait and the balance of 35 would be tested against a spread of masses, frictions and delays. **A controller is robust when it works over the range that the robot may be in.**

## Build It

`simulate_step` in `exercise/solution.hpp`.

```
rcpp verify 36-07
```

## Use It

`rc/legged/sim_to_real.hpp`.

```cpp
rc::legged::JointSim j;  j.delay = measured_delay;  j.lag = measured_lag;
const double margin = rc::legged::max_stable_delay(j, 0.5, 6.0, 0.001, 0.2) - measured_delay;
```

**What this does not do.** It is **one joint** with **linear parts and Coulomb friction**; the delay is **a whole number of time steps**, the lag **first order**, and **backlash, noise, quantisation of the encoder and the bus's jitter** are **not** in it. The **spread** is **three numbers drawn independently and evenly**; **real variation is correlated and not even**. **Settling in a step** is **one test** of a controller; a **whole gait** over **real ground** is another, and **only the robot** is the last word. **A simulator can be made more like the robot** (put the measured delay and lag into it), and the sim-to-real gap shrinks **by measurement**, not by hope. The numbers are **typical**.

## What Breaks First

- **Gains tuned in a simulator with no delay.** See `E-LEG-0070`.
- **A delay never measured on the real bus.** See `E-LEG-0071`.
- **A controller tested on one robot.** See `E-LEG-0069`.

## Ship It

`rc/legged/sim_to_real.hpp` holds `JointSim`, `StepResult`, `simulate_step`, `max_stable_delay` and `success_rate`.
