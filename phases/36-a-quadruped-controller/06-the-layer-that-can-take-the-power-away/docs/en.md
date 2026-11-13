# The Layer That Can Take the Power Away: Torque Limits, Heat Derating, a Link Watchdog, an Emergency Stop, and Why Off Is Not Always Safe

> Safety is a short list of rules that run every tick, outrank the controller, and know that a robot that is standing cannot simply be switched off.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 36-05, 36-02, 35-09, 31-08, 19-02

## The Problem

Everything so far assumes the controller **works**. 19-02 said what a **stop that does not need the app** is, and 27's vehicle lessons built the same idea for a ground vehicle; a quadruped needs its own, because **its failures are different**: its motors **overheat** (34-06), its **link** is a radio, it **falls** (35-09), and **taking the power away does not make it safe**: **a robot switched off while standing drops**. The **safety layer** is a **short list of rules** that run **every tick**, are **simple enough to be checked**, and **outrank** the controller.

## The Concept

### A limit on every torque

Every joint's command is **clipped to the peak the motor may give**: a command of **40 N m** was cut to **20**, either way. A controller that is **wrong** (a bad gain, a sign, a glitch in a sensor) can **ask** for anything; it cannot **get** more than the limit. See `E-LEG-0066`.

### Derating, then off, for heat

34-06's heat is a fraction of the limit. Below **0.8** the torque is **full**; between **0.8 and 1.0** it is **scaled down** linearly to **nothing**; at **1.0** the layer asks for **off**. Knees at **0.9** of their limit were derated to **0.50** of the torque, at **0.95** to **0.25**, and at **1.0** switched off. **A derated robot walks slowly and weakly instead of failing**, and the machine of 36-02 **refuses to rise** above 0.8. See `E-LEG-0067`.

### The link, the stop, the fall

A **watchdog** on the link (31-08's idea): silent for **0.3 s** the layer asked for **soft** (mode 2), silent for **2.0 s** for **off** (mode 3). The **emergency stop** and the **fallen** state ask for **off**; a fall that **cannot be stopped** (35-09) asks for **soft**: **2**. Every mode is **a number** (0 normal, 1 derated, 2 soft, 3 off), and the layer takes **the highest asked**: with the knees derated **and** the link silent for **0.3 s** it chose **2** (soft), and **kept the derating's scale**. See `E-LEG-0068`.

### Off is not always safe

**Off** means **the motors give no torque**. A robot that is **standing** and loses all torque **collapses**: 35-09's drop and its force of 1118 N on stiff legs. So **off while standing is turned into soft**: the legs **give gently** (low stiffness, damping kept) and the body **lowers**, and **only when it is down** does the layer go to **off**. The emergency stop with the robot **standing** gave **2** (soft), and **sitting 3** (off). **A real emergency stop on a legged robot is therefore two stages**, and a person who presses it needs to know the robot **will go down**. This is why a quadruped's stop is **not** the same as a wheeled vehicle's (19-02). See `E-LEG-0068`.

## Build It

`safety_decide` in `exercise/solution.hpp`.

```
rcpp verify 36-06
```

## Use It

`rc/legged/dog_safety.hpp`.

```cpp
const rc::legged::SafetyOut s = rc::legged::safety_decide(inputs, limits);
for (joint) torque[i] = rc::legged::limit_torque(rc::legged::apply_scale(cmd[i], s), tau_peak);
```

**What this does not do.** It is **a model of the layer's logic**, **not a certified safety function**: a real one runs on **separate hardware** (a watchdog that does not trust the main processor), with **proven timing** and a **hardware stop** that does not depend on software at all. The **thresholds** (0.1 s, 1.0 s, 0.8) are **numbers chosen here**. **Soft** is a **mode name**: its **effect** (the stiffness cut, the damping, the lowering) is **not worked out**. **Battery** and **current** limits, the **individual joint temperatures** (the hottest is used) and **sensor faults** (a tilt sensor that lies) are **not** in it. The robot **cannot know** it is standing without a sensor, and a **wrong** `standing` turns a stop into a drop or a hold.

## What Breaks First

- **A torque limit missing from the last stage.** See `E-LEG-0066`.
- **A hot robot with no derating.** See `E-LEG-0067`.
- **An emergency stop that drops the robot.** See `E-LEG-0068`.

## Ship It

`rc/legged/dog_safety.hpp` holds `SafeMode`, `SafetyIn`, `SafetyLimits`, `SafetyOut`, `limit_torque`, `safety_decide` and `apply_scale`.
