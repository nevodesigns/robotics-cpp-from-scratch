# Where the Weight Presses: The Centre of Pressure, the Zero Moment Point of an Accelerating Body, and the Acceleration the Feet Can Allow

> A body that accelerates presses on the ground behind its centre, and it stays up while that point stays inside the feet.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 35-05, 35-01, 33-05

## The Problem

35-01 asked whether the **centre of mass** was inside the feet, which is right for a body **at rest**. A body that **accelerates** is pushed by the **ground** at a point that is **not** under its centre of mass: accelerating forward, you lean forward, and the **weight presses further back** on the feet. The point that matters is where the ground's push **acts**, the **centre of pressure** (CoP), and the question for a moving body is whether it stays **inside the feet**. This is the **zero moment point** (ZMP), the idea of the walking-robot field, here in its simplest form.

## The Concept

### The centre of pressure

The **centre of pressure** is the mean of the feet's places weighted by the **vertical forces** on them: the point where the ground's total push acts. With equal forces on four feet it is at their centre. It **must lie inside the polygon of the feet on the ground**, because a foot cannot pull: the push of the ground is a sum of pushes at the feet and its point of action lies between them. See `E-LEG-0039`.

### The zero moment point of an accelerating body

For a body of height **h** that **accelerates horizontally** by **a** and **does not turn**, the **moment** of its inertia about the ground point (x, y) vanishes when the point is the **centre of mass moved back by h over g times a**. That point is where the CoP **has to be** for the body to be in balance. An acceleration of **3 m/s2** forward on a body **0.27 m** high moved it **83 mm** behind the centre of mass. The test loads the feet (with 35-05's force distribution) to put the CoP at that point and **the CoP came out at that point** to **0.000000000 m**: they are the same thing, and **ZMP is the name for where the CoP has to be**. See `E-LEG-0040`.

### How hard it may accelerate

The body is in balance while the ZMP is **inside the feet**, so the **greatest acceleration** in a direction is found by moving the ZMP out until it reaches the edge: with four feet it was **6.90 m/s2** forward, **6.90** backward and **5.45** sideways. These are **g times the distance from the centre to the edge, over the height** (the test checks the sideways one against 33-05's formula to the last digit: **1**). **A taller body accelerates less**, and a **wider base** more.

### With fewer feet

With a hind foot lifted and the body **centred**, the margin was **0.000 m** (35-01: the centre falls on the diagonal) and the acceleration allowed **0.00 m/s2**: **none**. Shifted toward the middle of the three feet, there is room. With **two diagonal feet** the support is a **line**, the margin is **0.000 m** at best, and **no acceleration is safe**: a trot is **not in balance in this sense**, it is **caught** (35-02). A **static** criterion fails for a trot; the **dynamic** one of 35-02 and 35-07 replaces it. See `E-LEG-0041`.

## Build It

`zmp_from_com` in `exercise/solution.hpp`.

```
rcpp verify 35-06
```

## Use It

`rc/legged/zmp.hpp`.

```cpp
double z[2];  rc::legged::zmp_from_com(com, height, accel, z);
if (rc::legged::support_margin(feet, on_ground, z) < 0.01) slow_down();
```

**What this does not do.** The body **does not turn** (a turning body adds its angular momentum's rate to the point); the **height** of the centre of mass is **constant**; the **feet do not slip** (friction, 23-05, limits the horizontal force too). The ground is **flat**. The **polygon's order** is the one of **four feet in a rectangle**. **Feet that are not on the ground** are the caller's to say (34-07). The numbers are **typical** of a 12 kg robot.

## What Breaks First

- **A balance check on the centre of mass alone.** See `E-LEG-0039`.
- **A sign error: the point put ahead and not behind.** See `E-LEG-0040`.
- **Static criteria applied to a trot.** See `E-LEG-0041`.

## Ship It

`rc/legged/zmp.hpp` holds `cop_from_forces`, `zmp_from_com`, `support_margin` and `max_accel`.
