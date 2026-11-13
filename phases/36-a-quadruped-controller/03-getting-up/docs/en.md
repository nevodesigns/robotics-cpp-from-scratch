# Getting Up: The Torque at the Lowest Crouch, the Heat of a Slow Rise, and the Heaviest Robot That Can Stand From the Ground

> Standing up from a crouch is the hardest thing the legs do all day, because the knees carry the most where they are strongest least.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 36-02, 34-06, 34-02

## The Problem

35-09 ended with a robot **on the ground**, and 36-02's machine has a state **fallen** that tries to **rise**, up to three times. Rising from a crouch is **the hardest thing the legs do all day**: the body is at its **lowest**, the legs are **most bent**, and the torque at the knee is **greatest where the motor is no stronger**. A robot that **cannot** rise, or can only by **cooking its knees**, must be told so **before** it tries, and the number of tries (36-02, `E-LEG-0056`) comes from this.

## The Concept

### The torque at the bottom

With **four feet down** the body's weight is shared four ways, and the knee's torque at each height is the **stance torque of 34-06**: the weight's share times how far the shank leans from the vertical (34-02). It is **greatest at the bottom of the rise** (the legs most folded, the shank leaning most) and **falls as the legs straighten** (the test checks three heights). A **12 kg** robot rising from a crouch of **0.12 m** to **0.28 m** needed up to **5.9 N m** at the knee, against a peak torque of **20 N m**. See `E-LEG-0057`.

### The heat of the rise

A rise is **a time at a high torque**. Each small step of height is counted as **its time over the time that torque would take to bring the winding to its limit** (29-08): the sum is the **share of the knee's thermal life** the rise uses (**one** means the limit is reached just as the rise ends). The rise in **1 s** used **0.034** of it; in **6 s**, **0.202**: **a slow rise is six times as hot as a fast one**, because the knee carries the weight for six times as long (a tidy result of this simple count, in which the rise's heat is just proportional to its time). So **rise promptly**. See `E-LEG-0058`.

### The heaviest that can rise

By halving an interval: the heaviest body that completes the rise **within the peak torque** was **40.6 kg**, **within the heat as well in 1 s** **40.6 kg**, and in **6 s** only **24.8 kg**: **a longer rise lowers what can be lifted** because of the heat. A **65 kg** robot could not rise at all (**0**, 1 for yes). A **shallower** crouch is easier (the test: a crouch of 0.20 m rises a heavier body than one of 0.08 m). See `E-LEG-0059`.

### What it means for the machine

36-02's machine **checks before it tries**: the **heat** is under 0.8 (34-06), the **battery** is enough, and **the plan says it can rise** (this lesson's `rise_plan` for the robot's mass **with whatever it carries**). A rise that **would** overheat the knees, or **needs more than the peak**, is **refused** and the machine goes to a fault (not another try). **Getting up from upside down** is a **different routine** (it begins with a roll over), and **not covered**.

## Build It

`rise_plan` in `exercise/solution.hpp`.

```
rcpp verify 36-03
```

## Use It

`rc/legged/get_up.hpp`.

```cpp
const rc::legged::RisePlan p = rc::legged::rise_plan(leg, mass, knee_motor, crouch_h, stand_h, 1.0, 40);
if (!p.torque_ok || !p.heat_ok) refuse_to_rise();
```

**What this does not do.** The rise is **quasi static** (no acceleration of the body, which needs **more** torque), with **four feet down** and the weight **shared equally**; the **start** is a **crouch with the feet under the hips**, not a body lying on its side. The **thermal count** is **Miner's rule** (a sum of shares), which treats a high torque for a short time as **using up a fraction of a life**, an **approximation** to the winding's real warming (29-08's lump). Only the **knee** is looked at. The motor numbers are **typical** of a hobby grade actuator.

## What Breaks First

- **A rise attempted with no check of the torque.** See `E-LEG-0057`.
- **A slow, careful rise.** See `E-LEG-0058`.
- **A heavy payload carried when it might fall.** See `E-LEG-0059`.

## Ship It

`rc/legged/get_up.hpp` holds `RisePlan`, `rise_plan` and `max_rising_mass`.
