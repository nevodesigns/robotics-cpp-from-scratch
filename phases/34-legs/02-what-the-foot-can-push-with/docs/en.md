# What the Foot Can Push With: The Leg Jacobian, the Joint Torques That Hold a Body Up, and Why a Bent Knee Costs More Than a Straight One

> The force at the foot and the torques at the joints are the same work seen from two ends, and the Jacobian is the lever between them.

**Type:** Build
**Time:** about 100 minutes
**Platforms:** Ubuntu 22.04 (GCC 11, Clang 14), Ubuntu 24.04, Windows
**Hardware:** none (a simulated robot)
**Prerequisites:** 34-01, 28-03, 29-01

## The Problem

34-01 said where the foot is. A leg stands and pushes: the ground pushes the **foot** up, and the **joints** have to give **torques** to hold the body. The force at the foot and the torques at the joints are **the same work seen from two ends**, and the matrix that connects them is the **Jacobian** of 28-03, now for a leg.

## The Concept

### The Jacobian of the leg

`leg_jacobian` is the 3 by 3 matrix that turns the **three joint rates** into the **foot's velocity** in the hip's frame: the derivative of `leg_fk` with respect to each joint. It was checked against small moves of each joint (and for a right leg too). See `E-LEG-0004`.

### Force and torque

By the principle of **virtual work**, a force that the ground pushes the foot with gives the joints a **generalised force** equal to the **Jacobian's transpose** times it, so the torques that **hold** it are **minus** that (the sign is the joints pushing back). A robot of **12 kg** on four legs puts **29.4 N** on each foot, and the torque at each joint follows with no dynamics at all.

### A bent knee costs

With the foot **0.20 m** below the hip (a crouch) the knee had to give **5.43 N m** and the hip pitch **0.00 N m** (the foot is directly under the hip, so the weight has no lever about it); at **0.30 m**, **4.33 N m** at the knee; and at **0.40 m**, with the leg nearly straight, **1.88 N m**. The knee's torque is the **weight times how far the shank leans from the vertical** (the test checks it), so it falls to nothing as the leg straightens: **a straight leg holds the body up on its bones and not on its motors**. A robot that stands on bent legs spends current, heat (34-06) and battery. See `E-LEG-0005`.

### Torques back to force

The other way, the force from measured joint torques, is the inverse of the transpose, solved by **Cramer's rule**: it gave the force back to **0.000000000 N**. It is how a leg **feels** the ground without a force sensor (34-07's contact sense comes from the same idea). It returns **false** where the determinant is nearly zero.

### The price of a straight leg

The inverse is the singularity again: a **foot speed of 0.5 m/s straight down** needed a knee rate of **3.4 rad/s** at **0.30 m** and **15.5 rad/s** at **0.40 m**, nearly straight, and a **fully straight** leg has **no** joint rates for it at all. So the leg is **strong** and **hard to control** where it is straight, and a robot keeps its legs **between** the two. See `E-LEG-0006`.

## Build It

`stand_torques` in `exercise/solution.hpp`.

```
rcpp verify 34-02
```

## Use It

`rc/legged/leg_force.hpp`.

```cpp
double j[3][3], tau[3];  rc::legged::leg_jacobian(leg, q0, q1, q2, j);
rc::legged::stand_torques(j, ground_force, tau);
```

**What this does not do.** It is **static**: the torques to hold a force, with no acceleration, no weight of the leg itself and no friction in the joints (29-04). The force is **vertical and shared equally** among four feet: the real sharing depends on where the body's centre of mass is, and **friction limits** the sideways force a foot may give (23-05). The legs' **masses** are zero. The numbers are **typical** of a 12 kg robot.

## What Breaks First

- **A Jacobian with the wrong side's sign.** See `E-LEG-0004`.
- **A crouch held for minutes.** See `E-LEG-0005`.
- **A foot sent along a straight leg.** See `E-LEG-0006`.

## Ship It

`rc/legged/leg_force.hpp` holds `leg_jacobian`, `stand_torques`, `det3`, `solve3`, `foot_force_from_torques` and `joint_rates`.
