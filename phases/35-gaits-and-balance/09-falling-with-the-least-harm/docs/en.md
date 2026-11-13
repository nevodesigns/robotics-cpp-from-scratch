# Falling With the Least Harm: How Long Before the Ground, Whether There Is Time to Recover, and Going Soft to Spread the Blow

> A robot that cannot stop a fall should decide early, and spend the time it has making the landing a gentle one.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 35-08, 35-07, 19-09

## The Problem

Whatever the controller, a legged robot **will fall** now and then: a bigger push than the feet and the step can answer (35-05, 35-07), a slope that was not what it looked like (35-08), a leg that failed. The last piece of balance is what to do **when the fall cannot be stopped**: **decide early**, and **spend the time that is left making the landing gentle** and the robot **safe to leave** (36-05 takes it up).

## The Concept

### How long before the ground

A tilting body is an **inverted pendulum**: **theta'' = (g / h) sin theta**. Stepping that equation from the **tilt and the tilt rate** gives the **time to the ground** (to a right angle). A body **0.27 m** up tilted **23 degrees** and turning at **3.0 rad/s** took **0.22 s**; one tilted **17 degrees** and turning at **0.2 rad/s** took **0.38 s**. It falls **sooner the larger the tilt and the rate**, and a body **swinging back toward upright fast enough** never gets there (the cap), which is a body **righting itself**. See `E-LEG-0048`.

### Recover, or go soft

A recovery **step** (35-07) needs time: here **0.30 s**, for the swing and the decision. So: **nothing** while the tilt and its rate are small (**the balance control holds it**; an inverted pendulum with any tilt at all falls in the end, so the time is meaningful only when the control is not holding it); **try to recover** if the time to the ground is **more than the step needs**; **go soft** if it is **less**; and **fallen** beyond a large tilt. The quick fall was told to **2** and the slow one **1** (0 nothing, 1 recover, 2 go soft, 3 fallen). It is a **decision made once and early**, not re-argued every tick: a robot that **tries to recover and goes soft too late** gets the worst of both. See `E-LEG-0049`.

### What soft is worth

When the fall cannot be stopped, the legs can be made **soft**: the stiffness of the joints cut and the **damping kept**, so they **give** as the body lands. The landing's energy (**mass times g times the drop**) is taken up over a **stroke**: the force is **energy over stroke**. A **12 kg** robot dropping **0.19 m** was hit with **1118 N** over a stroke of **0.02 m** (stiff legs) and **224 N** over **0.10 m** (soft ones), **5 times less**. A force that is five times smaller is the **difference between a scuffed shell and a broken servo**. See `E-LEG-0050`.

### After the fall

A fallen robot **should not get straight up**. Its **motors may be hot** (34-06), a **leg may be bent** or a **servo may have stripped**, and **getting up is one of the hardest things a legged robot does**. The **fallen** state hands over to the state machine of 36-02, which **checks before it tries**.

## Build It

`fall_action` in `exercise/solution.hpp`.

```
rcpp verify 35-09
```

## Use It

`rc/legged/fall_safety.hpp`.

```cpp
const int act = rc::legged::fall_action(tilt, tilt_rate, com_height, 0.30, 0.05, 0.5, 1.2);
if (act == rc::legged::kFallSoft) cut_stiffness_keep_damping();
```

**What this does not do.** The body is a **single pendulum about the feet**, in **one plane**, with **no legs' motion** and **no ground contact**; a real fall **rolls and twists**. The **drop** and the **stroke** are **given**: the real ones depend on **which part lands first**. **Going soft** is **a number** (the stiffness cut), and its **side effects** (the legs collapse, the body lands lower) are not worked out. **Recovery** is **assumed possible** if there is time; whether the step can be taken (35-07) is a separate question. **Getting up** is **not covered** here. The numbers are **typical** of a 12 kg robot.

## What Breaks First

- **A fall time never worked out.** See `E-LEG-0048`.
- **A recovery tried too late.** See `E-LEG-0049`.
- **A stiff robot that falls like a brick.** See `E-LEG-0050`.

## Ship It

`rc/legged/fall_safety.hpp` holds `FallAction`, `time_to_ground`, `fall_action` and `impact_force`. With it phase 35 is complete: a quadruped that walks and trots without falling over, with the support it has at each moment, the timing of its gaits and the change from one to another, an attitude it can trust, forces from the feet that keep the body level, the centre of pressure kept inside the feet, a step that answers a push, slopes and steps, and a fall that does the least harm.
